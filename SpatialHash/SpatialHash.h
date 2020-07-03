#pragma once

#include <random>
#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>
#include <math.h>
#include <cstdint>

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
/// Every thing in the Spatial Hash, every Entry, has, besides a place in 2d space, also an unique id.
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
/// So when a Entry is inserted to the Spatial Hash it's number in it's cell and it's hash value is saved.
/// </summary>
struct Entered
{
    Entry entry;
    uint32_t nrInCell;
    uint32_t hashValue;

    Entered(Entry inEntry, uint32_t inNrInCell, uint32_t inHashValue) : entry(inEntry), nrInCell(inNrInCell), hashValue(inHashValue) {}
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
/// GetCloseEntries returns this because it creates a array of EntryWithDistance but 
/// also needs to return how many elements there is in the array.
/// </summary>
struct CloseEntriesAndNrOf
{
    uint32_t nrOfEntries;
    EntryWithDistance* allCloseEntries;
};

/// <summary>
/// A Spatial Hash consists of these Cells
/// In localEntries all Entry:s that are inside of the cell in the sense of the hash-algorithm are stored.
/// The offsets contains pointers to Cells that are close to this one, precalculated for faster lookup. 
/// </summary>
struct Cell
{
    std::vector<Entered*>* localEntries;
    std::vector<uint32_t>* offsets;
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

    /* All the entries to the spatial hash is stored in this list,
     * rather then in the actual hash table, to save space and make the
     * table more cash friendly. */
    Entered* allEntries;

    /// <summary>
    /// After the class have been constructed this is called and will go through all the Entered's in
    /// *allEntries and insert them into the hash table.
    /// </summary>
    /// <param name="allEntries">Everything that is in the hash map should be in this array</param>
    /// <param name="numberOfEntries>The number of Entry:s in allEntries.</param>
    void Initilize(Entered* inAllEntries, uint32_t numberOfEntries);

    /// <summary>
    /// Checks all the Entered's in *allEntries to see if they have moved to a new cell. If they have
    /// moved, they are reinserted and the old one is removed.
    /// NOTE: Make sure you are inputting the correct number of Entry:s.
    /// </summary>
    /// <param name="numberOfEntries>The number of Entry:s in allEntries.</param>
    void UpdateTable(uint32_t numberOfEntries);

    /// <summary>
    /// Removes a entry from the hash table. NOTE: It does not remove them from *allEntries. That part
    /// is on whoever controls it.
    /// </summary>
    /// <param name="entry">Entry to remove from hash table.</param>
    void RemoveEntry(Entered* entry);

    /// <summary>
    /// Gets a number entites that are within a distance of a position.
    /// </summary>
    /// <param name="pos">Where to look for entites.</param>
    /// <param name="d">Radius of the search area.</param>
    /// <param name="maxEntities">No more than this number of entires will be returned.</param>
    /// <returns>Pointer to a list of entrise sorted by distance from input position.</returns>
    CloseEntriesAndNrOf GetCloseEntries(const Position position, float d, const unsigned short int maxEntities);

    /// <summary>
    /// The size in bytes of the elements of allEntries. This is so that which manages allEntries can
    /// allocate a an array that at least have the right size.
    /// </summary>
    /// <returns>The number of bytes in an Entered.</returns>
    uint32_t GetEnteredSize();

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

    // The size of the table needs to be (2^n x 2^n) for simpler realization of modulo function.
    const uint32_t sideLength;

    // Used for realisation of modulo function
    const int32_t xMask;
    const int32_t yMask;

    // Sorted list of entries to return from GetCloseEntities().
    std::vector<EntryWithDistance>* closeEntries;

    // Inserts a entry from *allEntries into the hash table.
    void InsertInTable(Entered* entry, uint32_t cellNr);
    void InsertInTable(Entered* entry);

    // Overloaded hash function.
    uint32_t CalculateCellNr(const Position pos);

    // Hash function.
    uint32_t CalculateCellNr(const float x, float y);

    // Called when constructing the class. Points all cells to their offsets.
    // TODO: Smarter way to generate the offsets. Put the initialization outside constructor?
    void InitializeOffsets();

    // Checks if input entity has moved cell and if so inserts it into the new and removes it from the old.
    void UpdateEntry(Entered* entry);
};
