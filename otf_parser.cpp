/* g++ -std=c++11 */
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <array>
#include <cstdint>
#include <cstring>
#include <cassert>

using std::ios;

#define FS_READ(var) reinterpret_cast<char *>(&var), sizeof(var)

const std::string TAG_CMAP = "cmap";


enum class VersionNum : uint32_t {
    V_CFF = 0x4F54544F,
    V_1_0 = 0x00010000
};


union header_version_u {
    char str[4];
    VersionNum num;
};


union tag_u {
    char str[4];
    uint32_t num;
};


struct Header {
    header_version_u version;
    uint16_t num_tables;
    uint16_t search_range;
    uint16_t entry_selector;
    uint16_t range_shift;
    std::string get_version_str() const;
};


struct Offset {
    tag_u tag;
    uint32_t checksum;
    uint32_t offset;
    uint32_t length;
    std::string get_tag_str() const;
};


struct OffsetTable {
    std::vector<Offset> entries;
    void add_entry(Offset &&offset);
    void add_entry(Offset &offset);
};


struct CmapHeader {
    uint16_t version;
    uint16_t num_tables;
    void endswap_self();
};


struct CmapEncodingRecord {
    uint16_t platform_id;
    uint16_t encoding_id;
    uint32_t offset;
    void endswap_self();
};


uint32_t calculate_checksum(Offset *table);


/**
 * Read in the otf file into header and array of offsets
 */
void read_in_otf(std::ifstream &otf_file, Header &header, OffsetTable &offset_table);


/**
 * Prints status of lil endian or big endian
 */
void print_debug_info();


/**
 * Prints debug message to say if this is CFF data
 */
void print_otf_version(const Header &header);


/**
 * Prints the otf header to stdout
 */
void print_otf_header(const Header &header);


/**
 * Print the contents of the offsets vector
 */
void print_offsets(const OffsetTable &offset_table);


int main(int argc, char **argv)
{
    if (argc > 1) {
        std::ifstream otf_file;
        otf_file.open(argv[1], ios::binary);

        if (otf_file.is_open())
            std::cout << "Reading file: " << argv[1] << std::endl;
        else
            return 1;

        Header header;
        OffsetTable offset_table;
        read_in_otf(otf_file, header, offset_table);

        print_debug_info();
        std::cout << std::endl;
        print_otf_header(header);
        std::cout << std::endl;
        print_offsets(offset_table);

        otf_file.close();

    } else {
        std::cerr << "Error: missing font file" << std::endl;
        return 1;
    }

    return 0;
} 

bool is_big_endian()
{
    const union {
        uint32_t i;
        char c[4];
    } bint = { 0x01020304 };
    return bint.c[0] == 1;
}

template <class T>
void endswap(T *objp)
{
    if (!is_big_endian()) {
        unsigned char *memp = reinterpret_cast<unsigned char*>(objp);
        std::reverse(memp, memp + sizeof(T));
    }
}

void endswap_otf_header(Header &header)
{
    endswap(&header.version);
    endswap(&header.num_tables);
    endswap(&header.search_range);
    endswap(&header.entry_selector);
    endswap(&header.range_shift);
}

void endswap_otf_offset(Offset &offset)
{
    //endswap(&offset.tag);
    endswap(&offset.checksum);
    endswap(&offset.offset);
    endswap(&offset.length);
}

uint32_t calculate_checksum(Offset *table)
{
    uint32_t sum = 0L;

    /*
    sum += table->tag;
    sum += table->offset;
    sum += table->length;
    */

    return sum;
}

void read_in_otf(std::ifstream &otf_file, Header &header, OffsetTable &offset_table)
{
    otf_file.read(FS_READ(header.version));
    otf_file.read(FS_READ(header.num_tables));
    otf_file.read(FS_READ(header.search_range));
    otf_file.read(FS_READ(header.entry_selector));
    otf_file.read(FS_READ(header.range_shift));
    endswap_otf_header(header);

    for (int i = 0; i < header.num_tables; i++) {
        Offset offset;

        otf_file.read(FS_READ(offset.tag));
        otf_file.read(FS_READ(offset.checksum));
        otf_file.read(FS_READ(offset.offset));
        otf_file.read(FS_READ(offset.length));
        endswap_otf_offset(offset);

        offset_table.add_entry(std::move(offset));
    }

    for (const auto &offset : offset_table.entries) {
        otf_file.seekg(offset.offset, ios::beg);

        /* Read in the cmap records */
        if (offset.get_tag_str() == TAG_CMAP) {
            CmapHeader cmap_header;
            otf_file.read(FS_READ(cmap_header.version));
            otf_file.read(FS_READ(cmap_header.num_tables));
            cmap_header.endswap_self();
            
            for (uint16_t i = 0; i < cmap_header.num_tables; i++) {
                CmapEncodingRecord encoding_record;
                otf_file.read(FS_READ(encoding_record.platform_id));
                otf_file.read(FS_READ(encoding_record.encoding_id));
                otf_file.read(FS_READ(encoding_record.offset));
                encoding_record.endswap_self();

                std::cout << "Platform id: " << encoding_record.platform_id << std::endl;
                std::cout << "encoding id: " << encoding_record.encoding_id << std::endl;
                std::cout << "offset: " << encoding_record.offset << std::endl;
            }
        }
    }

    otf_file.close();
}

void print_otf_version(const Header &header)
{
    if (header.version.num == VersionNum::V_CFF) {
        std::cout << "Contains CFF" << std::endl;
    }
}

void print_debug_info()
{
    if (is_big_endian()) {
        std::cout << "Is big endian" << std::endl;
    } else {
        std::cout << "Is little endian" << std::endl;
    }
}

void print_otf_header(const Header &header)
{
    int version_width = 7;
    int num_table_width = 18;
    int search_range_width = 13;
    int entry_selector_width = 15;
    int range_shift_width = 13;

    print_otf_version(header);

    std::cout << std::setw(version_width) <<  "Version"
              << std::setw(num_table_width) <<  "Number of tables"
              << std::setw(search_range_width) <<  "Search Range"
              << std::setw(entry_selector_width) <<  "Entry Selector"
              << std::setw(range_shift_width) <<  "Range Shift"
              << std::endl;

    std::cout << std::setw(version_width) <<  "-------"
              << std::setw(num_table_width) <<  "----------------"
              << std::setw(search_range_width) <<  "------------"
              << std::setw(entry_selector_width) <<  "--------------"
              << std::setw(range_shift_width) <<  "-----------"
              << std::endl;

    std::cout << std::setw(version_width) << header.get_version_str()
              << std::setw(num_table_width) << header.num_tables
              << std::setw(search_range_width) << header.search_range
              << std::setw(entry_selector_width) << header.entry_selector
              << std::setw(range_shift_width) << header.range_shift
              << std::endl;
}

void print_offsets(const OffsetTable &offset_table)
{
    int tag_width = 4;
    int checksum_width = 12;
    int offset_width = 8;
    int length_width = 8;

    std::cout << std::setw(tag_width) << "Tag"
              << std::setw(checksum_width) << "Checksum"
              << std::setw(offset_width) << "Offset"
              << std::setw(length_width) << "Length"
              << std::endl;

    std::cout << std::setw(tag_width) << "---"
              << std::setw(checksum_width) << "--------"
              << std::setw(offset_width) << "------"
              << std::setw(length_width) << "------"
              << std::endl;

    for (const auto &offset : offset_table.entries) {
        std::cout << std::setw(tag_width) << offset.get_tag_str()
                  << std::setw(checksum_width) << offset.checksum
                  << std::setw(offset_width) << offset.offset
                  << std::setw(length_width) << offset.length
                  << std::endl;
    }
}


/***
 * Header Implementation
 *
 */

std::string Header::get_version_str() const
{
    char version_str[5];
    strncpy(version_str, version.str, 4);
    version_str[4] = '\0';
    return std::string(version_str);
}


/***
 * OffsetTable Implementation
 *
 */

void OffsetTable::add_entry(Offset &&offset)
{
    entries.push_back(std::forward<Offset>(offset));
}

void OffsetTable::add_entry(Offset &offset)
{
    entries.push_back(offset);
}

std::string Offset::get_tag_str() const
{
    char tag_str[5];
    strncpy(tag_str, tag.str, 4);
    tag_str[4] = '\0';
    return std::string(tag_str);
}

/***
 * Cmap Implementation
 */

void CmapHeader::endswap_self()
{
    endswap(&version);
    endswap(&num_tables);
}

void CmapEncodingRecord::endswap_self()
{
    endswap(&platform_id);
    endswap(&encoding_id);
    endswap(&offset);
}
