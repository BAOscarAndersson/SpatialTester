#pragma once

#include <random>
#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>
#include <math.h>
#include <cstdint>
#include <fstream>

/// <summary>
/// Everything stored in the Spatial Hash have a 2d-coordinate, which is recorded as Position.
/// </summary>
struct Position
{
    float x;
    float y;

    Position() : x(0.0f), y(0.0f) {}
    Position(float inX, float inY) : x(inX), y(inY) {}
    ~Position() {}
};

/// <summary>
/// Every thing in the Spatial Hash, every Entry, has, besides a place in 2d space, also a id.
/// The id of an Entry is it's place in the allEntries array.
/// </summary>
struct Entry
{
    uint32_t id;
    Position position;

    Entry() : id(0), position() {}
    Entry(uint32_t inId, Position inPosition) : position(inPosition), id(inId) {}
    ~Entry() {}
};

/// <summary>
/// Each Entry needs to know it's place in the cell of the Spatial Hash and what cell it's in, 
/// for efficient removal.
/// So when a Entry is inserted in the Spatial Hash its number in the cell and hash value are saved.
/// </summary>
struct Entered
{
    Entry entry;
    uint32_t nrInCell;
    uint32_t hashValue;

    Entered(Entry inEntry, uint32_t inNrInCell, uint32_t inHashValue) :entry(inEntry), nrInCell(inNrInCell), hashValue(inHashValue) {}
    Entered() : entry(), nrInCell(0), hashValue(0) {}
    ~Entered() {}
};

/// <summary>
/// When entries close to a position is searched they need to be sorted by distance. Since this
/// information is needed later it's saved.
/// </summary>
struct EntryWithDistance
{
    Entry entry;
    float distance;

    EntryWithDistance(Entry inEntry, float inDistance) : entry(inEntry), distance(inDistance) {}
    EntryWithDistance() : entry(), distance(0.0f) {}

    ~EntryWithDistance() {}
};

/// <summary>
/// For more efficent interop the position is discarded from the return information. This means
/// that the calling program needs to store the positions if the need to do so.
/// </summary>
struct IdWithDistance
{
    uint32_t id;
    float distance;

    IdWithDistance(uint32_t inId, float inDistance) : id(inId), distance(inDistance) {}
    IdWithDistance() : id(0), distance(0.0f) {}

    ~IdWithDistance() {}
};

/// <summary>
/// GetCloseEntries returns this because it creates a array of EntryWithDistance but 
/// also needs to return how many elements there is in the array.
/// </summary>
struct CloseEntriesAndNrOf
{
    uint32_t* nrOfEntries;
    EntryWithDistance* allCloseEntries;
};

struct CloseIdsAndNrOf
{
    uint32_t* nrOfEntries;
    IdWithDistance* allCloseEntries;
};

/// <summary>
/// A Spatial Hash consists of these Cells
/// In localEntries all Entry:s that are inside of the cell in the sense of the hash-algorithm are stored.
/// The offsets contains pointers to Cells that are close to this one, precalculated for faster lookup. 
/// </summary>
struct Cell
{
    std::vector<Entry>* localEntries;
    std::vector<std::vector<int32_t>*>* offsets;
};

/// <summary>
/// The Spatial Hash stores objects from an float sized space in a relatively small hash table
/// that preserves the locality of objects. In the current implementation that is with a modulo function.
/// This function is realized with a bitwise AND operation for speed concerns and requires that
/// the hash table is of size 2^n for both dimensions.
/// </summary>
class SpatialHash
{
public:
    /// <summary>
    /// After the class have been constructed this is called and will go through all the Entered's in
    /// *allEntries and insert them into the hash table.
    /// </summary>
    /// <param name="allEntries">Everything that is in the hash map should be in this array</param>
    /// <param name="numberOfEntries>The number of Entry:s in allEntries.</param>
    void Initilize(Entry* inAllEntries, uint32_t numberOfEntries);

    /// <summary>
    /// Checks all the Entered's in *allEntries to see if they have moved to a new cell. If they have
    /// moved, they are reinserted and the old one is removed.
    /// NOTE: Make sure you are inputting the correct number of Entry:s.
    /// </summary>
    /// <param name="numberOfEntries>The number of Entry:s in allEntries.</param>
    void UpdateTable();

    /// <summary>
    /// Removes a entry from the hash table. NOTE: It does not remove them from *allEntries. That part
    /// is on whoever controls it.
    /// </summary>
    /// <param name="entry">Entry to remove from hash table.</param>
    void RemoveEntryFromTable(uint32_t entryIndex);

    /// <summary>
    /// Gets a number entites that are within a distance of a position.
    /// </summary>
    /// <param name="pos">Where to look for entites.</param>
    /// <param name="d">Radius of the search area.</param>
    /// <param name="maxEntities">No more than this number of entires will be returned.</param>
    /// <returns>Pointer to a list of entrise sorted by distance from input position.</returns>
    void GetCloseEntries(Position position, float d, int32_t maxEntities);

    CloseIdsAndNrOf GetCloseEntriesBulk(int32_t nrSearches, Position* positions, float d, int32_t maxEntities);

    /// <summary>
    /// Creates a square Spatial Hash table with length "size".
    /// allEntries is the array used to input Entries for insertion into the hash table.
    /// </summary>
    /// <param name="size">The lengths of the sides of the table.</param>
    SpatialHash(size_t size);

    ~SpatialHash();

private:

    // Actual hash table. Stores only pointers to the data of allEntries and minimumOffsets.
    std::vector<Cell>* table;

    /* All the entries to the spatial hash is stored in this list,
     * rather then in the actual hash table, to save space and make the
     * table more cash friendly. */
    Entry* allEntries;

   std::vector<Entered>* allEntered;

    uint32_t numberOfAllEntries;

    // The size of the table needs to be (2^n x 2^n) for simpler realization of modulo function.
    const uint32_t sideLength;

    float invCellSize;

    // Used for realisation of modulo function
    const uint32_t xMask;
    const uint32_t yMask;

    // Sorted list of entries to return, from GetCloseEntities().
    std::vector<IdWithDistance>* closeEntries;

    // Number of elements added to closeEntries for each GetCloseEntities().
    std::vector<uint32_t>* nrOfEntries;

    // GetCloseEntries will loop through this number of steps.
    uint32_t numberOfSteps;

    // Contains the unlocalized offsets. That is offsets that aren't adapted to any certain cell.
    std::vector<std::vector<int32_t>> xOffsetsToCalculate{};
    std::vector<std::vector<int32_t>> yOffsetsToCalculate{};

    // Total number of offsets in stepSizes, the offsets read from a file.
    uint32_t numberOfOffsets;

    // Stores all the offsets that the cells point to.
    std::vector<std::vector<int32_t>*>* globalOffsets;

    // Inserts a entry from *allEntries into the hash table.
    Entered InsertInTable(Entry* entry, uint32_t cellNr);
    Entered InsertInTable(Entry* entry);

    void RemoveEntryFromCell(Entered* entered);

    // Checks if input entity has moved cell and if so inserts it into the new and removes it from the old.
    void UpdateEntered(Entered* entered);

    void GetCloseEntriesInCell(uint32_t cellIndex, Position pos, float d);

    // Overloaded hash function.
    uint32_t CalculateCellNr(const Position pos);

    // Hash function.
    uint32_t CalculateCellNr(const float x, const float y);

    // Called when constructing the class. Points all cells to their offsets.
    // TODO: Smarter way to generate the offsets. Put the initialization outside constructor?
    void InitializeOffsets();

    void InitializeOffsetsInCell(uint32_t x, uint32_t y);
    void ReadOffsetsFromFile();

    // Used in GetCloseEntries
    void SortCloseEntries(int32_t from);
};
