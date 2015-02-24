/*
A module to read and write to an existing virtual disk
Supports only some sparse extents for now
*/

#include <iostream>
#include <fstream>
#include <stdint.h>
#include <vector>
#include <string>

typedef uint64_t SectorType;
typedef uint8_t Bool;
typedef struct SparseExtentHeader {
    uint32_t magicNumber;
    uint32_t version;
    uint32_t flags;
    SectorType capacity;
    SectorType grainSize;
    SectorType descriptorOffset;
    SectorType descriptorSize;
    uint32_t numGTEsPerGT;
    SectorType rgdOffset;
    SectorType gdOffset;
    SectorType overHead;
    Bool uncleanShutdown;
    char singleEndLineChar;
    char nonEndLineChar;
    char doubleEndLineChar1;
    char doubleEndLineChar2;
    uint16_t compressAlgorithm;
    uint8_t pad[433];
}__attribute__((__packed__)) SparseExtentHeader;

struct extent_type
{
    int num_sectors;
    std::fstream* file;
    SparseExtentHeader sph;
};

void file_open(std::fstream* file, const char* full_path)
{
    file->open(full_path, std::ios::in | std::ios::binary | std::ios::out);
    if (!file->good())
    {
        std::cout << "File " << full_path << " couldn't be opened" << std::endl;
    }
}

class vmdk_handler
{
private:
    extent_type* extents;
    int grain_table_coverage;

public:
    const int sector_size;

    vmdk_handler(const char* working_directory, const char* header_name)
    :sector_size(512)
    {
        std::string path = working_directory;
        path.append(header_name);
        std::fstream header;
        file_open(&header, path.c_str());

        path = working_directory;
        path.append("AusOS-s001.vmdk");
        extents[0].file = new std::fstream;
        file_open(extents[0].file, path.c_str());
        write_sector(0,0);
    }

    void read_header(extent_type* extent)
    {
        extent->file->read((char*)&extent->sph, sizeof(extent->sph));
        grain_table_coverage = extent->sph.numGTEsPerGT * extent->sph.grainSize * sector_size;
    }

    int find_grain(extent_type* extent, int sector_index, SectorType offset)
    {
        // get grain directory entry (grain table)
        extent->file->seekg((sector_index / grain_table_coverage + offset) * sector_size);
        int32_t grain_table_location;
        extent->file->read((char*) &grain_table_location, sizeof(grain_table_location));

        // get grain table entry (location of data)
        extent->file->seekg(grain_table_location * sector_size + (sector_index % grain_table_coverage / extent->sph.grainSize));
        int32_t grain_location;
        extent->file->read((char*) &grain_location, sizeof(grain_location));

        return grain_location;
    }

    int write_grain(extent_type* extent, int sector_index, SectorType offset, int32_t g_loc)
    {
        // get grain directory entry (grain table)
        extent->file->seekg((sector_index / grain_table_coverage + offset) * sector_size);
        int32_t grain_table_location;
        extent->file->read((char*) &grain_table_location, sizeof(grain_table_location));

        // get grain table entry (location of data)
        extent->file->seekg(grain_table_location * sector_size + (sector_index % grain_table_coverage / extent->sph.grainSize));
        int32_t grain_location = g_loc;
        extent->file->write((char*) &grain_location, sizeof(grain_location));

        return grain_location;
    }

    void read_sector(int id, int sector_index)
    {
        extent_type* extent = extents + id;
        read_header(extent);
        int grain_loc = find_grain(extent, sector_index, extent->sph.gdOffset);
        int redundant_grain_loc = find_grain(extent, sector_index, extent->sph.rgdOffset);
        if (grain_loc != redundant_grain_loc)
        {
            std::cout << "VMDK is corrupted" << std::endl;
        }
        // TODO actually read something
    }

    void write_sector(int id, int sector_index)
    {
        extent_type* extent = extents + id;
        read_header(extent);
        int grain_loc = find_grain(extent, sector_index, extent->sph.gdOffset);
        int redundant_grain_loc = find_grain(extent, sector_index, extent->sph.rgdOffset);
        if (grain_loc != redundant_grain_loc)
        {
            std::cout << "VMDK is corrupted" << std::endl;
        }
        extent->file->seekg(0, extent->file->end);
        char boot[256 * 512];
        for (int i = 0; i < 256 * 512; ++i) {
            boot[i] = 0;
        }
        boot[0] = 0xb4;
        boot[1] = 0x0e;
        boot[2] = 0xb0;
        boot[3] = 0x48; //H
        boot[4] = 0xcd;
        boot[5] = 0x10;
        boot[6] = 0xb0;
        boot[7] = 0x65; //e
        boot[8] = 0xcd;
        boot[9] = 0x10;
        boot[10] = 0xb0;
        boot[11] = 0x6c; //l
        boot[12] = 0xcd;
        boot[13] = 0x10;
        boot[14] = 0xb0;
        boot[15] = 0x6c; //l
        boot[16] = 0xcd;
        boot[17] = 0x10;
        boot[18] = 0xb0;
        boot[19] = 0x6f; //o
        boot[20] = 0xcd;
        boot[21] = 0x10;
        boot[22] = 0xb0;
        boot[23] = 0x20; // space
        boot[24] = 0xcd;
        boot[25] = 0x10;
        boot[26] = 0xb0;
        boot[27] = 0x57; // W
        boot[28] = 0xcd;
        boot[29] = 0x10;
        boot[30] = 0xb0;
        boot[31] = 0x6f; //o
        boot[32] = 0xcd;
        boot[33] = 0x10;
        boot[34] = 0xb0;
        boot[35] = 0x72; //r
        boot[36] = 0xcd;
        boot[37] = 0x10;
        boot[38] = 0xb0;
        boot[39] = 0x6c; //l
        boot[40] = 0xcd;
        boot[41] = 0x10;
        boot[42] = 0xb0;
        boot[43] = 0x64; //d
        boot[44] = 0xcd;
        boot[45] = 0x10;
        boot[46] = 0xe9;
        boot[47] = 0xfd;
        boot[48] = 0xff;
        boot[510] = 0x55;
        boot[511] = 0xaa;

        int32_t location = extent->file->tellg() / 512;
        extent->file->write(boot, 256 * 512);
        write_grain(extent, 0, extent->sph.gdOffset, location);
        write_grain(extent, 0, extent->sph.rgdOffset, location);
    }
};


int main()
{
    vmdk_handler vmdk("/home/austin/vmware/AusOS/", "AusOS.vmdk");
    return 0;
}
