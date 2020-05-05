#include "simulator.h"
#include "parser.h"
#include <fstream>

int main(int argc, char *argv[]) {
    std::ifstream in(argv[1]);
    auto input = Parser::parse_input_data(in);

    Channel channel(10, 10);
    Simulator simulator(input.total_page_count, input.access_history, channel);
    MigrationScheme migration_scheme;
    bool optimization_flag;
    std::string migration = argv[2];
    std::string opt = argv[3];
    if (migration == "pre") {
        migration_scheme = MigrationScheme::PreCopy;
    } else if (migration == "post") {
        migration_scheme = MigrationScheme ::PostCopy;
    } else {
        throw std::runtime_error("unknown migration");
    }
    if (opt == "1") {
        optimization_flag = true;
    } else if (opt == "0") {
        optimization_flag = false;
    } else {
        throw std::runtime_error("unknown optimization");
    }
    auto criterias = simulator.RunMigration(migration_scheme, optimization_flag);
    Parser::PrintOutput(criterias, std::cout);
}