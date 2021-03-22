/**
 * SQL.cpp
 * Author: Mark Minkoff
 * Desc: The source implementation of SQL.H.
 * CLI, Commands, Database Management
 */
#include "SQL.h"

SQL::SQL() : database_count(0)
{
    initializeCommands();
    initializeTypes();
    SQL_CLI();
}

SQL::~SQL()
{
    std::cout << "-- All done.\n";
}

void SQL::initializeCommands()
{
    // Specify command count and reserve memory in the unordered map for them
    std::size_t NUM_COMMANDS = 6;
    this->commands.reserve(NUM_COMMANDS);

    // Asign commands value pairs
    std::pair<std::string, unsigned int> cmd0("CREATE", 0);
    std::pair<std::string, unsigned int> cmd1("DROP",   1);
    std::pair<std::string, unsigned int> cmd2("USE",    2);
    std::pair<std::string, unsigned int> cmd3("ALTER",  3);
    std::pair<std::string, unsigned int> cmd4("SELECT", 4);
    std::pair<std::string, unsigned int> cmd5("INSERT", 5);

    // Insert commands into unordered map
    this->commands.insert(cmd0);
    this->commands.insert(cmd1);
    this->commands.insert(cmd2); 
    this->commands.insert(cmd3);
    this->commands.insert(cmd4);
    this->commands.insert(cmd5);
}

void SQL::SQL_CLI()
{
    // The user input
    std::string input;

    // Request user input from console
    std::getline(std::cin, input);

    // Check if the user input has balanced parenthsis, if not issue some error
    if (!parenthesisBalance(input))
    {
        std::cout << "-- !Parenthsis are not balanced in input: " << input << "\n";
        return SQL_CLI();
    }

    // The exit condition for the CLI
    if (input == ".EXIT" || input == "EXIT")
    {
        return;
    }

    // Split the user input with a space delimeter
    std::vector<std::string> args = SQL::split(input, ' ');

    HANDLE_CMD(args);

    return SQL_CLI();
}

bool SQL::dbSelected()
{
    // No database is selected if the database pointer is null
    if (this->database != nullptr) {
        return true;
    }
    return false;
}

bool SQL::createDatabase(const std::vector<std::string>& args)
{
    const unsigned int argn = args.size();

    // Maximum number of arguments for CREATE DATABASE command is three
    const unsigned int max_argn = 3;

    // Check if the number of argument supplied is less than the argument required
    if (argn < max_argn) { std::cout << "-- [CMD - CREATE - ERROR] -> Supplied argument count (" << argn << ") does not match required argument count (" << max_argn << ")\n"; return false; }

    const std::string command_name  = _toUpper(args[0]);
    const std::string database      = _toUpper(args[1]);
    const std::string database_name = args[2];

    // Check that the arguments are correct
    if (command_name != "CREATE" || database != "DATABASE") 
    { 
        std::cout << "-- !Programmer error in SQL::createTable. Contact admin :(\n";
        return false;
    }

    // Check if there are more supplied arguments than required
    if (argn > max_argn) { errorUnknownArguments(args, command_name, max_argn); return false; }
    
    // Check that the database has not been created, if so return.
    if(dbExists(database_name)) 
    {   
        std::cout << "-- !Failed to create database " << database_name << " because it already exists." << std::endl;
        return false;
    }
    try
    {
        // Crate the database and insert into the unordered map of databases
        this->databases.insert(std::make_pair(database_name, std::make_shared<Database>(database_name)));
        incrementDatabaseCount();
    }
    catch(const std::exception& e)
    {
        std::cerr << " -- In SQL::createDatabase => " << e.what() << '\n';
        return false;
    }
    
    std::cout << "-- Database " << database_name << " created.\n";
    return true;
}

bool SQL::dropDatabase(const std::vector<std::string>& args)
{
    const unsigned int argn = args.size();
    const unsigned int max_argn = 3;

    if (argn < max_argn) { std::cout << "-- [CMD - DROP - ERROR] -> Supplied argument count (" << argn << ") does not match required argument count (" << max_argn << ")\n"; return false; }

    const std::string command_name  = _toUpper(args[0]);
    const std::string database      = _toUpper(args[1]);
    const std::string database_name = args[2];

    // Check that the arguments are correct
    if (command_name != "DROP" || database != "DATABASE") 
    {
        std::cout << "-- !Programmer error in SQL::dropTable. Contact admin :(\n";
        return false;
    }

    if (argn > max_argn) { errorUnknownArguments(args, command_name, max_argn); return false; }

    // Check that the database has not been created, if so return.
    if(!dbExists(database_name)) 
    {   
        std::cout << "-- !Failed to delete database " << database_name << " because it does not exists." << std::endl;
        return false;
    }
    try
    {
        this->databases.erase(database_name);
        decrementDatabaseCount();
    }
    catch(const std::exception& e)
    {
        std::cerr << "In SQL::dropDatabase => " << e.what() << '\n';
        return false;
    }
    
    std::cout << "Database " << database_name << " deleted.\n";
    return true;
}

bool SQL::createTable(const std::vector<std::string>& args)
{
    const unsigned int argn = args.size();

    const std::string command = _toUpper(args[0]);
    const std::string table = _toUpper(args[1]);
    const std::string table_name = args[2];
    std::vector<std::pair<std::string, std::string>> columns;

    if (command != "CREATE" || table != "TABLE")
    {
        std::cout << "-- !Programmer error in SQL::createTable. Contact admin :(\n";
        return false;
    }
    

    if (!dbSelected())
    {
        std::cout << "-- !Failed to create table " << table_name << " because no database is selected.\n";
        return false;
    }

    if (this->database->tableExists(table_name))
    {
        std::cout << "-- !Failed to create table " << table_name << " because it already exists.\n";
        return false;
    }

    // Check if column names+types need to be parsed
    if (argn > 3)
    {
        std::vector<std::string> tableArgs(args.begin() + 3, args.end());
        columns = parseTableColumns(tableArgs);
        
        // If there are columns, preform type checking
        if (columns.size())
        {
            std::vector<std::string> types;
            for (auto& col : columns)
            {
                types.emplace_back(std::get<1>(col));
            }
            if (!checkTypes(types)) return false;
        }
    }

    this->database->createTable(table_name, columns);

    std::cout << "-- Table " << table_name << " created.\n";

    return true;
}

bool SQL::dropTable(const std::vector<std::string>& args)
{
    const unsigned int argn = args.size();

    const std::string command = _toUpper(args[0]);
    const std::string table = _toUpper(args[1]);
    const std::string table_name = args[2];

    if (command != "DROP" || table != "TABLE")
    {
        std::cout << "-- !Programmer error in SQL::dropTable. Contact admin :(\n";
        return false;
    }

    if (!dbSelected())
    {
        std::cout << "-- !Failed to drop table " << table_name << " because no database is selected.\n";
        return false;
    }

    if (!this->database->tableExists(table_name))
    {
        std::cout << "-- !Failed to drop table " << table_name << " because it does not exist.\n";
        return false;
    }

    this->database->dropTable(table_name);

    std::cout << "--Table " << table_name << " deleted.\n";

    return true;
}

bool SQL::useDatabase(std::shared_ptr<Database> db)
{
    if (db == nullptr) return false;

    this->database = db;

    std::cout << "-- Using database " << db->getDatabaseName() << ".\n";

    return true;
}

bool SQL::useDatabase(const std::vector<std::string>& args)
{
    const unsigned int argn = args.size();

    // Check for unexpected argument(s)
    if (argn > 2)
    {
        errorUnknownArguments(args, "USE", 2); // Handle unknown argument(s) error.
        return false;
    }

    if (args[0] != "USE")
    {
        std::cout << "-- !Programmer error in SQL::useDatabase, contact administrator.\n";
        return false;
    }

    const std::string database_name = args[1];

    // Check if provided database_name is empty
    if (database_name.size() == 0) 
    {
        std::cout << "-- !SQL::useDatabase provided empty database_name.\n";
        return false;
    }

    // Check if the database exists - if not it's an error.
    if (!dbExists(database_name))
    {
        std::cout << "-- !Database " << database_name << " does not exist.\n";
        return false;
    }

    std::shared_ptr<Database> db = getDatabase(database_name);
    if (!db) return false;

    return useDatabase(db);
}

bool SQL::HANDLE_CMD(std::vector<std::string> args)
{
    // Not the best handling of the semicolon lol but i'll eventually make a queue of commands
    // for now we can only run one command at a time
    if (args.back().back() == ';') { (args.back()).pop_back(); } 

    try {
        // Grab the commands from the provided arguments
        std::string command = _toUpper(args[0]);

        // Check if command exists, if it doesn't return 0
        if (!cmdExists(command)) {
            std::cout << "-- Command " << command << " does not exist.\n";
            return 0;
        }

        unsigned int command_id = cmdId(command);

        if (command_id == 0)     // CREATE COMMAND HANDLER
        {
            const std::string create_type = _toUpper(args[1]);
            if (create_type == "DATABASE") return createDatabase(args);
            else if (create_type == "TABLE") return createTable(args);
            else {
                std::cout << create_type << " is not a valid argument of command CREATE.\n";
                return false;
            }
        }
        else if (command_id == 1) // DROP COMMAND HANDLER
        {
            const std::string drop_type = _toUpper(args[1]);
            if (drop_type == "DATABASE") return dropDatabase(args);
            else if (drop_type == "TABLE") return dropTable(args);
            else 
            { 
                std::cout << drop_type << " is not a valid argument of command DROP.\n";
                return false;
            }
        }
        else if (command_id == 2) // USE COMMAND HANDLER
        {
            return useDatabase(args);
        }
        else if (command_id == 3) // ALTER COMMAND HANDLER
        {
            return alterTable(args);
        }
        else if (command_id == 4) // SELECT COMMAND HANDLER
        {
            return selectTable(args);
        }
        else if (command_id == 5) // INSERT COMMAND HANDLER
        {
            const std::string insert_type = _toUpper(args[1]);
            if (insert_type == "INTO") return insertInto(args);
            else {
                std::cout << "-- Invalid insert specifyer: " << insert_type << "\n";
                return false;
            }
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << "\n";
    }

    return true;
}

bool SQL::dbExists(const std::string& database_name)
{
    return this->databases.count(database_name);
}

bool SQL::cmdExists(const std::string& cmd)
{
    return (bool)this->commands.count(cmd);
}

unsigned int SQL::cmdId(const std::string& cmd)
{
    return this->commands.at(cmd);
}

std::vector<std::string> SQL::split(const std::string& s, char delimiter) 
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter))
    {
        tokens.push_back(token);
    }
    return tokens;
}

void SQL::errorUnknownArguments(const std::vector<std::string>& args, const std::string& cmd, unsigned int index)
{
    unsigned int argn = args.size();
    if (index > argn) return;
    std::cout << "-- [CMD-" << cmd << " - ERROR] -> Unknown Argument(s): {";
    for (; index < args.size(); index++) {
        std::cout << args[index];
        if (index < args.size()-1) std::cout << ", ";
    }
    std::cout << "}\n";
}

std::shared_ptr<Database> SQL::getDatabase(const std::string& database_name)
{
    if (!dbExists(database_name))
    {
        return nullptr;
    }

    return this->databases.at(database_name);
}

std::vector<std::pair<std::string, std::string>> SQL::parseTableColumns(std::vector<std::string> columns)
{
    unsigned int num_columns = columns.size();

    // Check if column arguments were provided.
    if (!num_columns)
    {
        // If not, return an empty column list
        std::cout << "-- !column arguments for CREATE TABLE do not exist\n";
        return std::vector<std::pair<std::string, std::string>>();
    }

    // Check if only two column arguments were provided
    if (num_columns == 2)
    {
        std::vector<std::pair<std::string, std::string>> res;
        std::string tempName = columns[0];
        std::string tempType = columns[1];
        unsigned int tempTypeSize = tempType.size();
        if (tempName[0] == '(') {
            columns[0].erase(columns[0].begin());
        }
        if (tempType.back() == ')' && (tempType[tempTypeSize - 2] == ')') && (!isdigit(tempType[tempTypeSize - 2])))
        {
            tempType.pop_back(); // maybe
        }

        res.emplace_back(std::make_pair(tempName, tempType));
        return res;
    }

    if ((num_columns % 2) != 0)
    {
        std::cout << "-- !Number of column arguments for CREATE TABLE are not even\n";
        // This means that the user inputed a missing name/type for column arguments
        return std::vector<std::pair<std::string, std::string>>();
    }

    // Check that the column arguments start and end with paranthesis
    if (! (columns[0][0] == '(') || ! (columns[num_columns-1].back() == ')'))
    {
        std::cout << "-- !Column arguments for CREATE TABLE are not wrapped with ()\n";
        return std::vector<std::pair<std::string, std::string>>();
    }
    
    // Delete the wrapped parenthesis
    columns[num_columns-1].pop_back();      // Last character of last string
    columns[0].erase(columns[0].begin());   // First character of first string

    std::string tempName;
    std::string tempType;
    std::vector<std::pair<std::string, std::string>> res;
    for (unsigned int index = 0; index < num_columns; ++index)
    {
        if (tempName.size() && tempType.size())
        {
            res.emplace_back(std::make_pair(tempName, tempType));
            tempName.clear(); tempType.clear();
        }

        if (index % 2) // 1, 3, 5, ...
        {
            tempType = columns[index];
            if (index < num_columns-1 && tempType.back() != ',')
            {
                std::cout << "-- !CREATE table error: Missing ',' after datatype " << tempType << ".\n";
                return std::vector<std::pair<std::string, std::string>>();
            }
            else if(columns.back() != tempType){
                tempType.pop_back();
            }
        }
        else // 0, 2, 4, 6
        {
            tempName = columns[index];
        }
    }
    if (tempName.size() && tempType.size())
    {
        res.emplace_back(std::make_pair(tempName, tempType));
    }

    return res;
}

bool SQL::parenthesisBalance(std::string s)
{
    std::stack<char> p;
        for (char& c : s) {
            switch (c) {
                case '(': 
                case '{': 
                case '[': p.push(c); break;
                case ')': if (p.empty() || p.top()!='(') return false; else p.pop(); break;
                case '}': if (p.empty() || p.top()!='{') return false; else p.pop(); break;
                case ']': if (p.empty() || p.top()!='[') return false; else p.pop(); break;
                default: ; 
            }
        }
        return p.empty();
}

bool SQL::selectTable(const std::vector<std::string>& args)
{
    const unsigned int argn = args.size();

    std::string command = _toUpper(args[0]);
    std::string select_type = _toUpper(args[1]);
    std::string from = _toUpper(args[2]);
    std::string table_name = args[3];


    if (command != "SELECT")
    {
        std::cout << "-- !Programmer error in SQL::selectTable. Contact admin :(\n";
        return false;
    }

    if (from != "FROM")
    {
        std::cout << "-- !Unknown argument from command SELECT: " << from << ". Did you mean FROM?\n";
    }

    if (argn > 4)
    {
        errorUnknownArguments(args, command, 4);
        return false;
    }

    if (!dbSelected())
    {
        std::cout << "-- !Failed to query table " << table_name << " because the database is not selected.\n";
    }

    if (!this->database->tableExists(table_name))
    {
        std::cout << "-- !Failed to query table " << table_name << " because it does not exist.\n";
    }

    if (select_type == "*") 
    {
        std::shared_ptr<Table> table = this->database->getTable(table_name);
        return table->printAll();
    }
    else {
        std::cout << "-- !Unknown argument from command SELECT: " << select_type <<  ". Did you mean '*' ?\n";
        return false;
    }
}

bool SQL::selectAllFromTable(const std::string& table_name)
{
    this->database->printTableColumnInfo(table_name);
    return true;
}

bool SQL::alterTable(const std::vector<std::string>& args)
{
    const unsigned int argn = args.size();
    const std::string command = args[0];
    const std::string table = args[1];
    const std::string table_name = args[2];
    const std::string alter_type = args[3];

    std::vector<std::string> tableArgs(args.begin() + 4, args.end());
    std::vector<std::pair<std::string, std::string>> columns = parseTableColumns(tableArgs);

    if (command != "ALTER" || table != "TABLE")
    {
        std::cout << "-- !Programmer error in SQL::alterTable. Contact admin :(\n";
        return false;
    }

    if (!database->tableExists(table_name))
    {
        std::cout << "-- !Could not modify table " << table_name << " because it did not exist.\n";
        return false;
    }

    this->database->addColumnsToTable(table_name, columns);

    std::cout << "-- Table " << table_name << " modified.\n";
    return true;
}

void SQL::initializeTypes()
{
    // Specify type count and reserve memory in the unordered map for them
    std::size_t NUM_TYPES = 4;
    this->types.reserve(NUM_TYPES);

    std::pair<std::string, unsigned int> type0("INT", 0);
    std::pair<std::string, unsigned int> type1("FLOAT", 1);
    std::pair<std::string, unsigned int> type2("CHAR", 2);
    std::pair<std::string, unsigned int> type3("VARCHAR", 3);

    // Insert commands into unordered map
    this->types.insert(type0);
    this->types.insert(type1);
    this->types.insert(type2); 
    this->types.insert(type3);
}

/*  From a list of types, check if any are invalid
    Print all invalid types 
    Return true if there are no errors
    Return false if there are errors */
bool SQL::checkTypes(std::vector<std::string> types)
{
    // Initialize errors container
    std::vector<std::string> errors;

    for (auto type: types)
    {
        std::string upperType = _toUpper(type);
        if (!this->types.count(upperType))
        {
            // Check if this is a VARCHAR and parathensis are formatted correctly
            if (upperType.substr(0, 8) == "VARCHAR(" && upperType.back() == ')')
            {
                // Comfirmed VARCHAR, now ensure the number is valid
                std::string num = upperType.substr(8, upperType.size() - 9);
                
                // Check if user entered a negative number
                if (num[0] == '-') 
                {
                    std::string temp = "-- TYPE ERORR: VARCHAR size (";
                    temp += num;
                    temp += ") cannot be negative.\n";
                    errors.emplace_back(temp);
                }
                // Check if any character is not valid for varchar size
                for (auto& c: num)
                {
                    if (!isdigit(c)) 
                    {
                        std::string temp = "-- TYPE ERROR: VARCHAR size (";
                        temp += num;
                        temp += ") contains non digit characters.\n";
                        errors.emplace_back(temp);
                    }
                }
            }
            else {
                std::string temp = "-- TYPE ERORR: Unknown type: ";
                temp += type;
                temp += '\n';
                errors.emplace_back(temp);
            }
        }
    }
    // If there are type errors, print them out and return false
    if (errors.size()) {
        for (auto& e: errors) {
            std::cout << e;
        }
        return false;
    }
    // Return true when there are no errors
    return true;
}

bool SQL::insertInto(const std::vector<std::string>& args)
{
    const std::string command = _toUpper(args[0]);
    const std::string command_type = _toUpper(args[1]);

    // Make sure we are handling the correct command
    if (command != "INSERT" && command_type != "INTO")
    {
        std::cout << "-- Programmer error in insertInto :(\n";
        return 0;
    }

    // Ensure a database is selected
    if (!dbSelected())
    {
        std::cout << "-- Database not selected\n";
        return false;
    }

    const std::string table_name = args[2];

    // Ensure table exists in the selected database
    const bool table_exists = this->database->tableExists(table_name);
    if (!table_exists)
    {
        std::cout << "-- Table " << table_name << " does not exist in database " << database->getDatabaseName() << "\n";
        return false;
    }

    // Create a sub vector of just the parameters
    std::vector<std::string> paramsVec = _subVector(args, args.begin() + 3, args.end());

    // Join the parameters as a string with space as delimeter
    std::string paramsString = _join(paramsVec, std::string(" "));

    // Ensure the user specified correct formatting
    std::string values = _toUpper(std::string(paramsString.begin() + 1, paramsString.begin() + 8));
    if (values != "VALUES(" || paramsString.back() != ')') 
    {
        std::cout << "-- INSERT INTO parameters not formatted correctly. Correct format is VALUES(x, y, z, ...)\n";
        return false;
    }

    // Parse the parameters we want to insert: parse format =  x,y,z,...
    std::string paramsParsed = std::string(paramsString.begin() + 8, paramsString.end() - 1);

    // Isolate the parameters without their delimeters
    std::vector<std::string> isolatedParams = isolateParams(paramsParsed);

    // Get a pointer to the table we want to insert into
    std::shared_ptr<Table> table = this->database->getTable(table_name);

    // Return the insertRow function in table
    bool success = table->insertRow(isolatedParams);

    if (!success) return false;

    std::cout << " -- 1 new record inserted.\n"; 

    return true;
}

std::vector<std::string> SQL::isolateParams(std::string params, char delim)
{
    // Initialize an empty vector container of strings for each parameter in params
    std::vector<std::string> container((size_t)1, std::string(""));

    // Initialize index to track each position of the container to insert characters into
    size_t index = 0;

    // Initialize stack to track if we are parsing quotes and therefore a varchar
    std::stack<char> quotes;

    // Iterate over every character of params, and parse each parameter
    for (auto& c : params) 
    {
        // The comma is a delimeter, so it marks the start of the next parameter unless we are parsing a varchar
        if (c == delim && quotes.empty()) 
        {
            ++index;
            container.emplace_back(std::string(""));
        }
        // Parsing the character " so insert into stack if applicable 
        else if (c == 34) 
        { 
            if (!quotes.empty() && quotes.top() == 34) quotes.pop();
            else if (quotes.empty() && (container[index].empty())) quotes.push(34);
            else container[index] += c;
        }
        // Parsing the character ' so insert into stack if applicable 
        else if (c == 39) 
        {
            if (!quotes.empty() && quotes.top() == 39) quotes.pop();
            else if (quotes.empty() && (container[index].empty())) quotes.push(39); 
            else container[index] += c;
        }
        // Valid character is appended into the container at index
        else {
            container[index] += c;
        }
    }

    return container;
}
