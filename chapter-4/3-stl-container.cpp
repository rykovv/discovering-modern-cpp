#include <map>
#include <iostream>

int main() {
    std::map<std::string, unsigned long long> phonebook = {
        {"bob", 123456789},
        {"alice", 987654321},
        {"randy", 134254678},
        {"mike", 84030275}
    };

    std::cout << "Existing name " << phonebook.at("randy") << std::endl;
    auto it = phonebook.find("abu");
    if (it == phonebook.end()) {
        std::cout << "Non-existing name is " << "abu" << std::endl;
    }

    unsigned long long existing_number = 134254678;
    it = find_if(phonebook.begin(), phonebook.end(),
        [&existing_number](auto pair) -> bool {
            return pair.second == existing_number;
        });
    if (it != phonebook.end()) {
        std::cout << "Existing number name is " << (*it).first << std::endl;
    }

    unsigned long long nonexisting_number = 0xbaaaaad;
    it = find_if(phonebook.begin(), phonebook.end(),
        [&nonexisting_number](auto pair) -> bool {
            return pair.second == nonexisting_number;
        });
    if (it == phonebook.end()) {
        std::cout << "There's no such number: " << nonexisting_number << std::endl;
    }

    return 0;
}