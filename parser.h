#pragma once

#include "simulator.h"
#include <vector>
#include <iostream>
#include <string>
#include <algorithm>
#include <unordered_map>

namespace Parser {
    struct InputData {
        std::vector<PageAccess> access_history;
        size_t total_page_count;
    };

    InputData parse_input_data(std::istream& in) {
        InputData input_data;

        std::string _;
        std::string op;
        Operation operation;
        size_t page_number;
        std::vector<size_t> page_numbers;
        while (in >> _ >> op >> std::hex >> page_number) {
            if (op == "R") {
                operation = Operation::Read;
            } else {
                operation = Operation::Write;
            }
            page_number = page_number >> 12;
            page_numbers.push_back(page_number);
            input_data.access_history.push_back({page_number, operation});
        }

        std::sort(page_numbers.begin(), page_numbers.end());
        page_numbers.erase(std::unique(page_numbers.begin(), page_numbers.end()), page_numbers.end());
        std::unordered_map<size_t, size_t> page_to_index;
        for (size_t i = 0; i < page_numbers.size(); ++i) {
            page_to_index[page_numbers[i]] = i;
        }
        for (auto& it : input_data.access_history) {
            it.page_number = page_to_index.at(it.page_number);
        }

        input_data.total_page_count = page_to_index.size();

        return input_data;
    }

    void PrintOutput(Criterias criterias, std::ostream &out) {
        out << "downtime: " << criterias.downtime << '\n';
        out << "eviction time: " << criterias.eviction_time << '\n';
        out << "total_migration time: " << criterias.total_migration_time << '\n';
        out << "transmitted data: " << criterias.transmitted_data << '\n';
        out << "delays: " << criterias.delays << '\n';
    }
}