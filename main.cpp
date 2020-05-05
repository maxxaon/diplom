#include "simulator.h"
#include "parser.h"
#include <fstream>

int main() {
    std::ifstream in("input.txt");
    auto input = Parser::parse_input_data(in);

    Channel channel(10, 10);
    Simulator simulator(input.total_page_count, input.access_history, channel);
    auto criterias = simulator.RunMigration(MigrationScheme::PostCopy, true);
    Parser::PrintOutput(criterias, std::cout);
}