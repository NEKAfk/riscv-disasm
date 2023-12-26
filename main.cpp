#include<fstream>
#include<iostream>
#include<limits>
#include<unordered_map>
#include <stdint.h>
#include <string>

using namespace std;
#define uint32_t uint64_t

#define E_IDENT 16
int unmarked = 0;
uint64_t ind = 0;
unordered_map<uint32_t, string> symbols;
uint32_t* buffer;

uint32_t getNBytes(int n) {
    uint32_t res = 0;
    uint32_t pow16 = 1;
    for (int i = 0; i < n; i++) {
        res += buffer[ind] * pow16;
        pow16 *= 256;
        ind++;
    }
    return res;
}

void skipNBytes(int n) {
    ind+=n;
}

string getString(uint32_t begin, uint32_t index) {
    string tmp = "";
    for (int j = begin + index; buffer[j] != 0; j++) {
        tmp.push_back((char)buffer[j]);
    }
    return tmp;
}

int iType(uint32_t command) {
    return (int)((command >> 20) + (command >> 31) * 0xFFFFF000);
}

int sType(uint32_t command) {
    uint32_t val1 = (command >> 7) & 0x1F;
    uint32_t val2 = command >> 25;
    return (int)(val1 + (val2 << 5) + (val2 >> 6) * 0xFFFFF000);
}

int bType(uint32_t command) {
    uint32_t val1 = (command >> 7) & 0x1F;
    uint32_t val2 = command >> 25;
    return (int)((val1 >> 1) + ((val2 & 0x3F) << 4) + ((val1 & 1) << 10) + (val2 >> 6) * (0xFFFFF000 >> 1)) << 1;
}

uint32_t uType(uint32_t command) {
    return command >> 12;
}

int jType(uint32_t command) {
    uint32_t val = command >> 12;
    return (int)((((val >> 9) & 0x3FF) + (((val >> 8) & 1) << 10) + ((val & 0xFF) << 11) + (val >> 19) * (0xFFF00000 >> 1)) << 1);
}

string fence(int mask) {
    string res = "";
    if (mask >> 3) {
        res.push_back('i');
    }
    if ((mask >> 2) & 1) {
        res.push_back('o');
    }
    if ((mask >> 1) & 1) {
        res.push_back('r');
    }
    if (mask & 1) {
        res.push_back('w');
    }
    return res;
}

void initSymbols(uint32_t e_shoff, uint32_t e_shnum, uint32_t e_shentsize, uint32_t nameIndex, uint32_t snameIndex) {
    ind = e_shoff;
    uint32_t sh_name = getNBytes(4);
    uint32_t sh_type = getNBytes(4);

    int i = 0;
    while (sh_type != 2 && i < e_shnum) {
        skipNBytes(e_shentsize - 8);
        sh_name = getNBytes(4);
        sh_type = getNBytes(4);
        i++;
    }
    if (i == e_shnum) {
        return;
    }
    uint32_t sh_flags = getNBytes(4);
    if ((sh_flags >> 1) & 1 != 1) {
        return;
    }
    uint32_t sh_addr = getNBytes(4);
    uint32_t sh_offset = getNBytes(4);
    uint32_t sh_size = getNBytes(4);
    uint32_t sh_link = getNBytes(4);
    uint32_t sh_info = getNBytes(4);
    uint32_t sh_addralign = getNBytes(4);
    uint32_t sh_entsize = getNBytes(4);
    skipNBytes(e_shentsize - 40);

    ind = sh_offset;
    for (int i = 0; (i+1)*sh_entsize <= sh_size; i++) {
        uint32_t st_name = getNBytes(4);
        uint32_t st_value = getNBytes(4);
        skipNBytes(4 + 1 + 1 + 2);

        uint32_t value = st_value;
        string name = getString(nameIndex, st_name);
        symbols[value] = name;
    }
}

void parseSymTab(uint32_t e_shoff, uint32_t e_shnum, uint32_t e_shentsize, uint32_t nameIndex, uint32_t snameIndex) {
    ind = e_shoff;
    uint32_t sh_name = getNBytes(4);
    uint32_t sh_type = getNBytes(4);

    int i = 0;
    while (sh_type != 2 && i < e_shnum) {
        skipNBytes(e_shentsize - 8);
        sh_name = getNBytes(4);
        sh_type = getNBytes(4);
        i++;
    }
    if (i == e_shnum) {
        return;
    }
    uint32_t sh_flags = getNBytes(4);
    if ((sh_flags >> 1) & 1 != 1) {
        return;
    }
    uint32_t sh_addr = getNBytes(4);
    uint32_t sh_offset = getNBytes(4);
    uint32_t sh_size = getNBytes(4);
    uint32_t sh_link = getNBytes(4);
    uint32_t sh_info = getNBytes(4);
    uint32_t sh_addralign = getNBytes(4);
    uint32_t sh_entsize = getNBytes(4);
    skipNBytes(e_shentsize - 40);

    ind = sh_offset;
    printf("%s\n", getString(snameIndex, sh_name).c_str());
    printf("\n%s\n", "Symbol Value              Size Type     Bind     Vis       Index Name");
    for (int i = 0; (i+1)*sh_entsize <= sh_size; i++) {
        uint32_t st_name = getNBytes(4);
        uint32_t st_value = getNBytes(4);
        uint32_t st_size = getNBytes(4);
        uint32_t st_info = getNBytes(1);
        uint32_t st_other = getNBytes(1);
        uint32_t st_shndx = getNBytes(2);
        uint32_t st_bind = st_info>>4;
        uint32_t st_type = st_info & 15;
        uint32_t st_vis = st_other & 3;

        uint32_t value = st_value;
        uint32_t size = st_size;
        string index;
        string bind;
        string type;
        string vis;
        string name = getString(nameIndex, st_name);
        
        //index
        switch (st_shndx) {
            case 0:
                index = "UNDEF";
                break;
            case 65521:
                index = "ABS";
                break;
            default:
                index = to_string(st_shndx).c_str();
                break;
        }

        //bind
        switch (st_bind) {
            case 0:
                bind = "LOCAL";
                break;
            case 1:
                bind = "GLOBAL";
                break;
            case 2:
                bind = "WEAK";
                break;
            case 13:
                bind = "LOPROC";
                break;
            case 15:
                bind = "HIPROC";
                break;
        }

        //type
        switch (st_type) {
            case 0:
                type = "NOTYPE";
                break;
            case 1:
                type = "OBJECT";
                break;
            case 2:
                type = "FUNC";
                break;
            case 3:
                type = "SECTION";
                break;
            case 4:
                type = "FILE";
                break;
            case 13:
                type = "LOPROC";
                break;
            case 15:
                type = "HIPROC";
                break;
        }
    
        //vis
        switch (st_vis) {
            case 0:
                vis = "DEFAULT";
                break;
            case 1:
                vis = "INTERNAL";
                break;
            case 2:
                vis = "HIDDEN";
                break;
            case 3:
                vis = "PROTECTED";
                break;
            case 4:
                vis = "EXPORTED";
                break;
            case 5:
                vis = "SINGLETON";
                break;
            case 6:
                vis = "ELIMINATE";
                break;
        }
        printf("[%4i] 0x%-15X %5i %-8s %-8s %-8s %6s %s\n", i, value, size, type.c_str(), bind.c_str(), vis.c_str(), index.c_str(), name.c_str());
    }
}

string parseReg(uint32_t reg) {
    if (reg == 0) {
        return "zero";
    } else if (reg == 1) {
        return "ra";
    } else if (reg == 2) {
        return "sp";
    } else if (reg == 3) {
        return "gp";
    } else if (reg == 4) {
        return "tp";
    } else if (reg <= 7) {
        return "t" + to_string(reg - 5);
    } else if (reg == 8) {
        return "s0";
    } else if (reg == 9) {
        return "s1";
    } else if (reg <= 17) {
        return "a" + to_string(reg - 10);
    } else if (reg <= 27) {
        return "s" + to_string(reg - 16);
    } else {
        return "t" + to_string(reg - 25);
    }
}

void parseCommand(uint32_t command, uint32_t addr) {
    cout << dec;
    switch (command & 0x7F) {
        case 0x13:
            switch((command >> 12) & 0x7) {
                case 0x0:
                    printf("%7s\t", "addi");
                    printf("%s, %s, %s\n", parseReg((command >> 7) & 0x1F).c_str(), parseReg((command >> 15) & 0x1F).c_str(), to_string(iType(command)).c_str());
                    break;
                case 0x2:
                    printf("%7s\t", "slti");
                    printf("%s, %s, %s\n", parseReg((command >> 7) & 0x1F).c_str(), parseReg((command >> 15) & 0x1F).c_str(), to_string(iType(command)).c_str());
                    break;
                case 0x3:
                    printf("%7s\t", "sltiu");
                    printf("%s, %s, %s\n", parseReg((command >> 7) & 0x1F).c_str(), parseReg((command >> 15) & 0x1F).c_str(), to_string(iType(command)).c_str());
                    break;
                case 0x4:
                    printf("%7s\t", "xori");
                    printf("%s, %s, %s\n", parseReg((command >> 7) & 0x1F).c_str(), parseReg((command >> 15) & 0x1F).c_str(), to_string(iType(command)).c_str());
                    break;
                case 0x6:
                    printf("%7s\t", "ori");
                    printf("%s, %s, %s\n", parseReg((command >> 7) & 0x1F).c_str(), parseReg((command >> 15) & 0x1F).c_str(), to_string(iType(command)).c_str());
                    break;
                case 0x7:
                    printf("%7s\t", "andi");
                    printf("%s, %s, %s\n", parseReg((command >> 7) & 0x1F).c_str(), parseReg((command >> 15) & 0x1F).c_str(), to_string(iType(command)).c_str());
                    break;
                case 0x1:
                    if ((command >> 25) != 0) {
                        printf("%-7s\n", "invalid_instruction");
                        return;
                    }
                    printf("%7s\t", "slli");
                    printf("%s, %s, %s\n", parseReg((command >> 7) & 0x1F).c_str(), parseReg((command >> 15) & 0x1F).c_str(), to_string((command >> 20) & 0x1F).c_str());
                    break;
                case 0x5:
                    if ((command >> 25) == 0) {
                        printf("%7s\t", "srli");
                    } else if ((command >> 25) == 0x20) {
                        printf("%7s\t", "srai");
                    } else {
                        printf("%-7s\n", "invalid_instruction");
                        return;
                    }
                    printf("%s, %s, %s\n", parseReg((command >> 7) & 0x1F).c_str(), parseReg((command >> 15) & 0x1F).c_str(), to_string((command >> 20) & 0x1F).c_str());
                    break;
                default:
                    printf("%-7s\n", "invalid_instruction");
                    return;
            }
            break;
        case 0x33:
            switch ((command >> 12) & 0x7) {
                case 0:
                    if ((command >> 25) == 0) {
                        printf("%7s\t", "add");
                    } else if ((command >> 25) == 0x1) {
                        printf("%7s\t", "mul");
                    } else if ((command >> 25) == 0x20) {
                        printf("%7s\t", "sub");
                    } else {
                        printf("%-7s\n", "invalid_instruction");
                        return;
                    }
                    printf("%s, %s, %s\n", parseReg((command >> 7) & 0x1F).c_str(), parseReg((command >> 15) & 0x1F).c_str(), parseReg((command >> 20) & 0x1F).c_str());
                    break;
                case 1:
                    if ((command >> 25) == 0) {
                        printf("%7s\t", "sll");
                    } else if ((command >> 25) == 0x1) {
                        printf("%7s\t", "mulh");
                    } else {
                        printf("%-7s\n", "invalid_instruction");
                        return;
                    }
                    printf("%s, %s, %s\n", parseReg((command >> 7) & 0x1F).c_str(), parseReg((command >> 15) & 0x1F).c_str(), parseReg((command >> 20) & 0x1F).c_str());
                    break;
                case 2:
                    if ((command >> 25) == 0) {
                        printf("%7s\t", "slt");
                    } else if ((command >> 25) == 0x1) {
                        printf("%7s\t", "mulhsu");
                    } else {
                        printf("%-7s\n", "invalid_instruction");
                        return;
                    }
                    printf("%s, %s, %s\n", parseReg((command >> 7) & 0x1F).c_str(), parseReg((command >> 15) & 0x1F).c_str(), parseReg((command >> 20) & 0x1F).c_str());
                    break;
                case 3:
                    if ((command >> 25) == 0) {
                        printf("%7s\t", "sltu");
                    } else if ((command >> 25) == 0x1) {
                        printf("%7s\t", "mulhu");
                    } else {
                        printf("%-7s\n", "invalid_instruction");
                        return;
                    }
                    printf("%s, %s, %s\n", parseReg((command >> 7) & 0x1F).c_str(), parseReg((command >> 15) & 0x1F).c_str(), parseReg((command >> 20) & 0x1F).c_str());
                    break;
                case 4:
                    if ((command >> 25) == 0) {
                        printf("%7s\t", "xor");
                    } else if ((command >> 25) == 0x1) {
                        printf("%7s\t", "div");
                    } else {
                        printf("%-7s\n", "invalid_instruction");
                        return;
                    }
                    printf("%s, %s, %s\n", parseReg((command >> 7) & 0x1F).c_str(), parseReg((command >> 15) & 0x1F).c_str(), parseReg((command >> 20) & 0x1F).c_str());
                    break;
                case 5:
                    if ((command >> 25) == 0) {
                        printf("%7s\t", "srl");
                    } else if ((command >> 25) == 0x1) {
                        printf("%7s\t", "divu");
                    } else if ((command >> 25) == 0x20) {
                        printf("%7s\t", "sra");
                    } else {
                        printf("%-7s\n", "invalid_instruction");
                        return;
                    }
                    printf("%s, %s, %s\n", parseReg((command >> 7) & 0x1F).c_str(), parseReg((command >> 15) & 0x1F).c_str(), parseReg((command >> 20) & 0x1F).c_str());
                    break;
                case 6:
                    if ((command >> 25) == 0) {
                        printf("%7s\t", "or");
                    } else if ((command >> 25) == 0x1) {
                        printf("%7s\t", "rem");
                    } else {
                        printf("%-7s\n", "invalid_instruction");
                        return;
                    }
                    printf("%s, %s, %s\n", parseReg((command >> 7) & 0x1F).c_str(), parseReg((command >> 15) & 0x1F).c_str(), parseReg((command >> 20) & 0x1F).c_str());
                    break;
                case 7:
                    if ((command >> 25) == 0) {
                        printf("%7s\t", "and");
                    } else if ((command >> 25) == 0x1) {
                        printf("%7s\t", "remu");
                    } else {
                        printf("%-7s\n", "invalid_instruction");
                        return;
                    }
                    printf("%s, %s, %s\n", parseReg((command >> 7) & 0x1F).c_str(), parseReg((command >> 15) & 0x1F).c_str(), parseReg((command >> 20) & 0x1F).c_str());
                    break;
                default:
                    printf("%-7s\n", "invalid_instruction");
                    return;
            }
            break;
        case 0x6F:
            printf("%7s\t%s, 0x%x <%s>\n", "jal", parseReg((command >> 7) & 0x1F).c_str(), addr + jType(command), symbols[addr + jType(command)].c_str());
            break;
        case 0x67:
            if ((command >> 12) & 0x7 != 0) {
                printf("%-7s\n", "invalid_instruction");
                return;
            }
            printf("%7s\t%s, %d(%s)\n", "jalr", parseReg((command >> 7) & 0x1F).c_str(), iType(command), parseReg((command >> 15) & 0x1F).c_str());
            break;
        case 0x37:
            printf("%7s\t%s, 0x%x\n", "lui", parseReg((command >> 7) & 0x1F).c_str(), uType(command));
            break;
        case 0x17:
            printf("%7s\t%s, 0x%x\n", "auipc", parseReg((command >> 7) & 0x1F).c_str(), uType(command));
            break;
        case 0x63:
            switch ((command >> 12) & 0x7) {
                case 0:
                    printf("%7s\t", "beq");
                    break;
                case 1:
                    printf("%7s\t", "bne");
                    break;
                case 4:
                    printf("%7s\t", "blt");
                    break;
                case 5:
                    printf("%7s\t", "bge");
                    break;
                case 6:
                    printf("%7s\t", "bltu");
                    break;
                case 7:
                    printf("%7s\t", "bgeu");
                    break;
                default:
                    printf("%-7s\n", "invalid_instruction");
                    return;
            }
            printf("%s, %s, 0x%x, <%s>\n", parseReg((command >> 15) & 0x1F).c_str(), parseReg((command >> 20) & 0x1F).c_str(), addr + bType(command), symbols[addr + bType(command)].c_str());
            break;
        case 0x3:
            switch ((command >> 12) & 0x7) {
                case 0:
                    printf("%7s\t", "lb");
                    break;
                case 1:
                    printf("%7s\t", "lh");
                    break;
                case 2:
                    printf("%7s\t", "lw");
                    break;
                case 4:
                    printf("%7s\t", "lbu");
                    break;
                case 5:
                    printf("%7s\t", "lhu");
                    break;
                default:
                    printf("%-7s\n", "invalid_instruction");
                    return;
            }
            printf("%s, %d(%s)\n", parseReg((command >> 7) & 0x1F).c_str(), iType(command), parseReg((command >> 15) & 0x1F).c_str());
            break;
        case 0x23:
            switch ((command >> 12) & 0x7) {
                case 0:
                    printf("%7s\t", "sb");
                    break;
                case 1:
                    printf("%7s\t", "sh");
                    break;
                case 2:
                    printf("%7s\t", "sw");
                    break;
                default:
                    printf("%-7s\n", "invalid_instruction");
                    return;
            }
            printf("%s, %d(%s)\n", parseReg((command >> 20) & 0x1F).c_str(), sType(command), parseReg((command >> 15) & 0x1F).c_str());
            break;
        case 0x73:
            if ((command >> 7) == 0) {
                printf("%7s\n", "ecall");
            } else if ((command >> 7) == 0x2000) {
                printf("%7s\n", "ebreak");
            } else {
                printf("%-7s\n", "invalid_instruction");
                return;
            }
            break;
        case 0xF:
            if ((command >> 12) & 0x7 != 0) {
                printf("%-7s\n", "invalid_instruction");
                return;
            }
            if (command == 0x0100000F) {
                printf("%7s\n", "pause");
            } else if ((command & 0xF00FFFF0) == 0) {
                printf("%7s\t%s, %s\n", "fence", fence((command >> 24) & 0xF).c_str(), fence((command >> 20) & 0xF).c_str());
            } else {
                printf("%-7s\n", "invalid_instruction");
                return;
            }
            break;
        default:
            printf("%-7s\n", "invalid_instruction");
            return;
    }
}

void initMap(uint32_t command, uint32_t addr) {
    int tmp = -1;
    switch (command & 0x7F) {
        case 0x6F:
            tmp = addr + jType(command);
            break;
        case 0x63:
            if ((command >> 12) & 0x7 == 2 || (command >> 12) & 0x7 == 3) {
                return;
            }
            tmp = addr + bType(command);
            break;
        default:
            return;
    }
    if (tmp == -1 || symbols.count(tmp)) {
        return;
    }
    symbols[tmp] = "L" + to_string(unmarked);
    unmarked++;
}

void parseText(uint32_t e_shoff, uint32_t e_shnum, uint32_t e_shentsize, uint32_t snameIndex) {
    ind = e_shoff;
    uint32_t sh_name = getNBytes(4);
    uint32_t sh_type = getNBytes(4);
    uint32_t sh_flags = getNBytes(4);
    int i = 0;
    while (sh_type != 1 && ((sh_flags >> 1) & 3) != 3 && i < e_shnum) {
        skipNBytes(e_shentsize - 12);
        sh_name = getNBytes(4);
        sh_type = getNBytes(4);
        uint32_t sh_flags = getNBytes(4);
        i++;
    }
    if (i == e_shnum) {
        return;
    }
    uint32_t sh_addr = getNBytes(4);
    uint32_t sh_offset = getNBytes(4);
    uint32_t sh_size = getNBytes(4);
    uint32_t sh_link = getNBytes(4);
    uint32_t sh_info = getNBytes(4);
    uint32_t sh_addralign = getNBytes(4);
    uint32_t sh_entsize = getNBytes(4);
    printf("%s\n", getString(snameIndex, sh_name).c_str());

    ind = sh_offset;
    uint32_t addr = sh_addr;
    uint32_t command = getNBytes(4);
    for (uint32_t i = 1; i < sh_size / 4; i++)
    {
        initMap(command, addr);
        command = getNBytes(4);
        addr += 4;
    }

    ind = sh_offset;
    addr = sh_addr;
    command = getNBytes(4);
    for (uint32_t i = 1; i <= sh_size / 4; i++)
    {
        if (symbols.count(addr)) {
            printf("\n%08x \t<%s>:\n", addr, symbols[addr].c_str());
        }
        printf("   %05x:\t%08x\t", addr, command);
        parseCommand(command, addr);
        command = getNBytes(4);
        addr += 4;
    }    
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "input mismatch";
        return argc;
    }
    freopen(argv[2], "w", stdout);
    #pragma region init
    ifstream fin(argv[1], ios_base::binary);
    fin.seekg(0, ios_base::end);
    int length = fin.tellg();
    fin.seekg(0, ios_base::beg);
    buffer = new uint32_t[length];
    #pragma endregion

    #pragma region HEADER
    for (uint32_t i = 0; i < length; i++)
    {
        buffer[i] = fin.get();
    }
    ind+=16;

    skipNBytes(2+2+4+4+4); //e_type, e_machine, e_version, e_entry, e_phoff
    uint32_t e_shoff = getNBytes(4);
    skipNBytes(4+2+2+2); //e_flags, e_ehsize, e_phentsize, e_phnum
    uint32_t e_shentsize = getNBytes(2);
    uint32_t e_shnum = getNBytes(2);
    uint32_t e_shstrndx = getNBytes(2);
    #pragma endregion

    if (e_shnum == 0) {
        ind = e_shoff;
        skipNBytes(5 * 4);
        e_shnum = getNBytes(4);
    }
    if (e_shstrndx == 0xffff) {
        ind = e_shoff;
        skipNBytes(6 * 4);
        e_shstrndx = getNBytes(4);
    }
    
    ind = e_shoff + e_shstrndx * e_shentsize;
    skipNBytes(4 * 4);
    uint32_t e_shstroffset = getNBytes(4);

    uint32_t sh_name = 0;
    uint32_t sh_symoffset = 0;
    ind = e_shoff;
    int i = 0;
    while (getString(e_shstroffset, sh_name) != ".strtab" && i < e_shnum) {
        sh_name = getNBytes(4);
        skipNBytes(3 * 4);
        sh_symoffset = getNBytes(4);
        skipNBytes(e_shentsize - 5 * 4);
        i++;
    }
    initSymbols(e_shoff, e_shnum, e_shentsize, sh_symoffset, e_shstroffset);
    parseText(e_shoff, e_shnum, e_shentsize, e_shstroffset);
    printf("\n\n");
    parseSymTab(e_shoff, e_shnum, e_shentsize, sh_symoffset, e_shstroffset);
}