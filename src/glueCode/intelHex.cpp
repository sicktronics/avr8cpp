#include "intelHex.h"


void loadHex(const std::string& source, std::vector<uint8_t>& target) {
    std::istringstream stream(source);
    std::string line;

  target.assign(0x8000 * 2, 0);


    while (std::getline(stream, line)) {
        // Trim leading whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        if (line[0] == ':' && line.substr(7, 2) == "00") {
            int bytes = std::stoi(line.substr(1, 2), nullptr, 16);
            int addr = std::stoi(line.substr(3, 4), nullptr, 16);

            for (int i = 0; i < bytes; i++) {
                int value = std::stoi(line.substr(9 + i * 2, 2), nullptr, 16);
                // std::cout << value << std::endl;
                if (addr + i >= target.size()) {
                    throw std::out_of_range("Address exceeds target size");
                }
                target[addr + i] = static_cast<uint8_t>(value);
                // std::cout << "Address: " << addr << ", Value: " << value << std::endl;
            }
        }
    }
}

/*
export function loadHex(source: string, target: Uint8Array) {
  for (const line of source.split('\n')) {
    if (line[0] === ':' && line.substr(7, 2) === '00') {
      const bytes = parseInt(line.substr(1, 2), 16);
      const addr = parseInt(line.substr(3, 4), 16);
      for (let i = 0; i < bytes; i++) {
        target[addr + i] = parseInt(line.substr(9 + i * 2, 2), 16);
      }
    }
  }
}
*/