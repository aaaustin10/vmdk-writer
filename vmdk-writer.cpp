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
        read_sector(0,0);
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
};


int main()
{
    vmdk_handler vmdk("/home/austin/vmware/AusOS/", "AusOS.vmdk");
    return 0;
}
