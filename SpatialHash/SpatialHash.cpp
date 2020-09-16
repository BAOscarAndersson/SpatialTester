#include "pch.h"
#include "SpatialHash.h"

using namespace std;

// Interop declarations.
extern "C" __declspec(dllexport) void* Start(uint32_t tableSize);
extern "C" __declspec(dllexport) void Init(uint32_t nrEntries, Entry * globalEntries, SpatialHash * spatialHash);
extern "C" __declspec(dllexport) uint32_t Stop(SpatialHash * spatialHash);
extern "C" __declspec(dllexport) CloseIdsAndNrOf GetEntries(int32_t nrOfPositions, Position* position, float d, int32_t maxEntities, SpatialHash * spatialHash);
extern "C" __declspec(dllexport) void Update(SpatialHash * spatialHash);
extern "C" __declspec(dllexport) void Remove(uint32_t nrOfEntriesToRemove, uint32_t * entryIndices, SpatialHash * spatialHash);

/// <summary>
/// Proper modulo function.
/// </summary>
inline int ProperMod(const uint32_t a, const int b)
{
    return (a < 0 ? (((a % b) + b) % b) : (a % b));
}

/// <summary>
/// Euclidian distance.
/// </summary>
inline float Distance(const Position a, const Position b)
{
    return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));
}

/// <summary>
/// Creates a Spatial Hash of a certain size.
/// </summary>
/// <param name="sideLength">The size of the Spatial Hash, needs to be a power of two.</param>
SpatialHash::SpatialHash(size_t sidePower) : allEntries(allEntries), sideLength(pow(2, sidePower)), xMask(sideLength-1), yMask(sideLength-1)
{
    // Temporary value that will probably be determined at runtime later.
    constexpr uint32_t reservedLocalEntries = 16;

    invCellSize = 1 / (float)sideLength;

    // table represents a two dimensional square.
    table = new vector<Cell>();
    table->resize(sideLength * sideLength);

    for (uint32_t i = 0; i != table->size(); i++)
    {
        table->at(i).localEntries = new vector<Entry>();
        table->at(i).localEntries->reserve(reservedLocalEntries);

        table->at(i).offsets = new vector<vector<int32_t>*>();

        globalOffsets = new vector<vector<int32_t>*>();
        globalOffsets->reserve(100);
    }
    
    closeEntries = new vector<IdWithDistance>();
    nrOfEntries = new vector<uint32_t>();

    allEntered = new vector<Entered>();
    numberOfAllEntries = 0;
}

SpatialHash::~SpatialHash()
{
    for (uint32_t i = 0; i != globalOffsets->size(); i++)
    {
        delete globalOffsets->at(i);
    }

    delete globalOffsets;

    for (size_t i = 0; i != table->size(); i++)
    {
        delete table->at(i).localEntries;
        delete table->at(i).offsets;
    }

    delete table;

    delete closeEntries;
}

/// <summary>
/// Inserts all the entries in allEntries into the hash table.
/// </summary>
void SpatialHash::Initilize(Entry* inAllEntries, uint32_t numberOfEntries)
{
    InitializeOffsets();

    allEntries = inAllEntries;

    numberOfAllEntries = numberOfEntries;

    allEntered->resize(numberOfAllEntries);

    for (uint32_t i = 0; i < numberOfAllEntries; i++)
    {
        allEntered->at(i) = InsertInTable(&allEntries[i]);
    }
}

/// <summary>
/// Checks if any entry in allEntries have moved to another cell,
/// in which case it inserts them into that cell and removes them
/// from their old one.
/// </summary>
void SpatialHash::UpdateTable()
{
    for (uint32_t i = 0; i < numberOfAllEntries; i++)
    {
        allEntered->at(i).entry = allEntries[i];
        UpdateEntered(&allEntered->at(i));
    }
}

/// <summary>
/// Checks if inputed entry have moved to a new cell. If it has
/// moved, it is reinserted and the old one is removed.
/// </summary>
/// <param name="entered">Entry will be put into correct cell.</param>
void SpatialHash::UpdateEntered(Entered* entered)
{
    uint32_t currenthashValue = CalculateCellNr(entered->entry.position);
    if (currenthashValue != entered->hashValue)
    {
        RemoveEntryFromCell(entered);
        allEntered->at(entered->entry.id) = InsertInTable(&entered->entry, currenthashValue);
    }
    else
    {
        table->at(entered->hashValue).localEntries->at(entered->nrInCell) = entered->entry;
    }
}

void SpatialHash::RemoveEntryFromTable(uint32_t entryIndex)
{
}

void SpatialHash::RemoveEntryFromTableBulk(uint32_t nrOfEntriesToRemove, uint32_t* entryIndices)
{
    for (uint32_t i = 0; i < nrOfEntriesToRemove; i++)
    {
        RemoveEntryFromTable(entryIndices[i]);
    }
}

/// <summary>
/// Removes input entry from hash table.
/// </summary>
/// <param name="entered">Entry to remove from its current cell.</param>
void SpatialHash::RemoveEntryFromCell(Entered* entered)
{
    // If the entry is at the end of the vector it can be removed straight away.
    if (entered->nrInCell == table->at(entered->hashValue).localEntries->size())
    {
        table->at(entered->hashValue).localEntries->pop_back();
    }
    else
    {
        // If the entry isn't at the end of the vector, it's overwritten with the entry that is.
        allEntered->at(table->at(entered->hashValue).localEntries->back().id).nrInCell = entered->nrInCell;
        table->at(entered->hashValue).localEntries->at(entered->nrInCell) = table->at(entered->hashValue).localEntries->back();
        table->at(entered->hashValue).localEntries->pop_back();
    }
}

/// <summary>
/// Inserts an entry into the spatial hash table.
/// </summary>
/// <param name="entry">Entry to insert.</param>
/// <returns>The entry with the information of where in the hash table it is.</returns>
Entered SpatialHash::InsertInTable(Entry* entry)
{
    uint32_t cellNr = CalculateCellNr(entry->position);
    return SpatialHash::InsertInTable(entry, cellNr);
}
Entered SpatialHash::InsertInTable(Entry* entry, uint32_t cellNr)
{
    this->table->at(cellNr).localEntries->push_back(*entry);

    return Entered(*entry, this->table->at(cellNr).localEntries->size()-1, cellNr);
}

/// <summary>
/// The hashing function. Calculates where in the spatial hash a position ends up.
/// </summary>
/// <param name="pos">The position to find a cell for.</param>
/// <returns>The cell number associated with the input position.</returns>
uint32_t SpatialHash::CalculateCellNr(Position pos)
{
    return CalculateCellNr(pos.x, pos.y);
}

/// <summary>
/// The hashing function. Calculates where in the spatial hash a position ends up.
/// </summary>
/// <param name="x">Horizontal position to find a cell for.</param>
/// <param name="y">Vertical position to find a cell for.</param>
/// <returns>The cell number associated with the input position.</returns>
uint32_t SpatialHash::CalculateCellNr(float x, float y)
{
    // Calculate where in the theoretical 2d hash table the entry would end up.
    float tx = x * invCellSize;
    float ty = y * invCellSize;
    
    uint32_t xCellNr = static_cast<uint32_t>(tx) & xMask;
    uint32_t yCellNr = static_cast<uint32_t>(ty) & yMask;

    // Convert this to the actual cell in the vector.
    return xCellNr + yCellNr * sideLength;
}

/// <summary>
/// Gets all entities in the spatial hash that are within a certain distance of a position.
/// </summary>
/// <param name="pos">Where to search for entities.</param>
/// <param name="d">The radius of the search area.</param>
/// <param name="maxEntities">The max number of entities to return.</param>
void SpatialHash::GetCloseEntries(Position pos, float d, int32_t maxEntities)
{
    // This is the cell that will be the origo of the search.
    uint32_t cellNr = CalculateCellNr(pos);

    /* Loops through the different steps. If you find enough close entities
    in a step, you can end the loop and return since there can be no other closer
    entites. */
    for (uint32_t i = 0; i < table->at(cellNr).offsets->size() ; i++)
    {
        int32_t currentStart = closeEntries->size();

        // Loops through all the offsets that belong to the current step.
        for (uint32_t j = 0; j < table->at(cellNr).offsets->at(i)->size(); j++)
        {
            uint32_t offsetCell = cellNr + table->at(cellNr).offsets->at(i)->at(j);

            GetCloseEntriesInCell(offsetCell, pos, d);
        }

        // If there are too many entries only the closest should be kept so they need to be orderd.
        SortCloseEntries(currentStart);

        // If there's enough elements the function can return.
        if (closeEntries->size() >= maxEntities)
        {
            closeEntries->resize(maxEntities);
            nrOfEntries->push_back(static_cast<uint32_t>(closeEntries->size()));
            return;
        }
    }

    nrOfEntries->push_back(static_cast<uint32_t>(closeEntries->size()));

    return;
}

/// <summary>
/// Gets all the entries in a cell of the hash table that are within a distance, d, of a position, pos.
/// </summary>
/// <param name="cellIndex">Cell to look for close entries in.</param>
/// <param name="pos">Position to look for close entries around.</param>
/// <param name="d">The distance inside of which entries are considered close.</param>
inline void SpatialHash::GetCloseEntriesInCell(uint32_t cellIndex, Position pos, float d)
{
    float tempDistance = 0;
    Entry tempEntry;
    IdWithDistance tempIdWithDistance;

    for (size_t m = 0; m < table->at(cellIndex).localEntries->size(); m++)
    {
        tempDistance = Distance(table->at(cellIndex).localEntries->at(m).position, pos);

        /* Imporant because entites sharing cells can still be very far from each other because
         * of the hashing/modulo on insertion. */
        if (tempDistance < d)
        {
            tempEntry = table->at(cellIndex).localEntries->at(m);
            tempIdWithDistance = IdWithDistance(tempEntry.id, tempDistance);

            closeEntries->push_back(tempIdWithDistance);
        }
    }
}

/// <summary>
/// Sorts the elements of closeEntries which is needed for GetCloseEntries
/// to be able to tell which elements are the closests.
/// </summary>
/// <param name="from">From which index to sort the list.</param>
inline void SpatialHash::SortCloseEntries(int32_t from)
{
    /* Insert-sort the vector. The whole point of the Spatial Hash is that there should only be
     * a small number of elements in this list so using Insert Sort probably makes sense.
     * This should be tested sometime though. */
    for (int32_t h = from; h < static_cast<int>(closeEntries->size()); h++)
    {
        IdWithDistance tempEntry = closeEntries->at(h);

        int32_t k = h - 1;
        while (k >= from && closeEntries->at(k).distance > tempEntry.distance)
        {
            closeEntries->at(k + 1) = closeEntries->at(k);
            k--;
        }

        closeEntries->at(k + 1) = tempEntry;
    }
}

/// <summary>
/// For more efficent interoping GetCloseEntries requests are bunched together.
/// </summary>
/// <param name="nrSearches">How many GetCloseEntries requests that are bunched together.</param>
/// <param name="pos">An array of postions, nrSearches long.</param>
/// <param name="d">Distance in which entries are considered close.</param>
/// <param name="maxEntities">Max number of entries per GetCloseEntries to return.</param>
/// <returns>The entries that are close to the positions and how many of them there are.</returns>
CloseIdsAndNrOf SpatialHash::GetCloseEntriesBulk(int32_t nrSearches, Position* pos, float d, int32_t maxEntities)
{
    // Since new entries to return is to be calculated we need to get rid of the old ones.
    closeEntries->clear();
    closeEntries->reserve(nrSearches*maxEntities);

    nrOfEntries->clear();
    nrOfEntries->reserve(nrSearches);

    for (int32_t i = 0; i < nrSearches; i++)
    {
        // Max entities for GetCloseEntries have to take into account previously added entries.
        SpatialHash::GetCloseEntries(pos[i], d, maxEntities + closeEntries->size());
    }

    if (nrSearches == 0)
    {
        nrOfEntries->push_back(0);
    }

    return CloseIdsAndNrOf{ nrOfEntries->data(), closeEntries->data() };
}

/// <summary>
/// Calculates all the different localized offsets from unlocalized offsets read from a file.
/// These are then stored in globalOffsets.
/// </summary>
void SpatialHash::InitializeOffsets()
{
    ReadOffsetsFromFile();

    for (int32_t y = 0; y != sideLength; y++)
    {
        for (int32_t x = 0; x != sideLength; x++)
        {
            InitializeOffsetsInCell(x, y);
        }
    }
}

/// <summary>
/// Calculates all the different offsets of a cell and points them to their
/// representation in globalOffsets or if the isn't in there puts it there.
/// </summary>
/// <param name="x"></param>
/// <param name="y"></param>
void SpatialHash::InitializeOffsetsInCell(uint32_t x, uint32_t y)
{
    uint32_t i = x + y * sideLength;

    vector<int32_t>* cellOffsets;

    // Loop through all the steps.
    for (uint32_t k = 0; k < xOffsetsToCalculate.size(); k++)
    {
        cellOffsets = new vector<int32_t>();

        // Loop through all the offsets of the current step.
        for (uint32_t j = 0; j < xOffsetsToCalculate[k].size(); j++)
        {
            // Offsets are calculated from the unlocalized offsets.
            int32_t tx = x + xOffsetsToCalculate[k][j];
            int32_t ty = y + yOffsetsToCalculate[k][j];
            tx = ProperMod(tx, sideLength);
            ty = ProperMod(ty, sideLength);

            cellOffsets->push_back((tx + ty * sideLength) - i);
        }

        bool found = false;

        // Loop through all the offsets in global offsets and check if the offset already exist.
        for (uint32_t j = 0; j < globalOffsets->size(); j++)
        {
            if (*globalOffsets->at(j) == *cellOffsets)
            {
                delete cellOffsets;
                table->at(i).offsets->push_back(globalOffsets->at(j));
                found = true;
                break;
            }
        }

        if (!found)
        {
            globalOffsets->push_back(cellOffsets);
            table->at(i).offsets->push_back(cellOffsets);
        }
    }
}

/// <summary>
/// The unlocalized offsets are calculated in another program and saved as a file since they don't need to be calculated more than once.
/// </summary>
void SpatialHash::ReadOffsetsFromFile()
{
    // The offsets are calculated before runtime and saved in a file that is loaded here.
    ifstream offsetsFile;
    offsetsFile.open("F:\\Prog\\Repos\\SpaceFiller\\bin\\Debug\\netcoreapp3.1\\Offsets.abi", ios::in | ios::binary);

    if (offsetsFile.is_open())
    {
        uint32_t numberOfSteps = 0;

        vector<int32_t> xOffsets;
        vector<int32_t> yOffsets;

        offsetsFile.read((char*)&numberOfSteps, sizeof(numberOfSteps));

        for (uint32_t i = 0; i < numberOfSteps; i++)
        {
            int32_t stepSize = 0;
            offsetsFile.read((char*)&stepSize, sizeof(stepSize));
            
            xOffsets.clear();
            yOffsets.clear();

            for (uint32_t j = 0; j < stepSize; j++)
            {
                uint32_t x = 0;
                offsetsFile.read((char*)&x, sizeof(x));
                xOffsets.push_back(x);

                uint32_t y = 0;
                offsetsFile.read((char*)&y, sizeof(y));
                yOffsets.push_back(y);
            }

            xOffsetsToCalculate.push_back(xOffsets);
            yOffsetsToCalculate.push_back(yOffsets);
        }

        offsetsFile.close();
    }
}

/*-------------INTEROPS------------*/

/// <summary>
/// Instanciates a SpatialHash of a certain size.
/// </summary>
/// <param name="tableSize">The length and height of the table will be 2^tableSize.</param>
/// <returns>A pointer to the instanciated SpatialHash.</returns>
void* Start(uint32_t tableSize)
{
    SpatialHash* spatialHash = new SpatialHash(tableSize);

    return spatialHash;
}

/// <summary>
/// Loads the SpatialHash with the start number of entries via an array of entries.
/// Also initilizes the offsets of the Spatial Hash.
/// </summary>
/// <param name="nrEntries">Number of entries in the globalEntries array.</param>
/// <param name="globalEntries">An array of Entry structs.</param>
/// <param name="spatialHash">Which SpatialHash to intilize.</param>
void Init(uint32_t nrEntries, Entry* globalEntries, SpatialHash* spatialHash)
{
    spatialHash->Initilize(globalEntries, nrEntries);
}

/// <summary>
/// Have to be called or may the Theoretical Gods have mercy on my soul.
/// </summary>
/// <param name="spatialHash">Which Spatial Hash to stop.</param>
/// <returns>0 if it ran to completion.</returns>
uint32_t Stop(SpatialHash* spatialHash)
{
    delete spatialHash;

    return 0;
}

/// <summary>
/// Retrives entries close to input positions from the Spatial Hash.
/// </summary>
/// <param name="nrPositions">Number of positions to search.</param>
/// <param name="position">Positions to check for close entries, An array of size nrPositions.</param>
/// <param name="d">How far away from the position entries can be to be close.</param>
/// <param name="maxEntities">Maximum number of entries to return per search.</param>
/// <param name="spatialHash">Which Spatial Hash to look in.</param>
/// <returns>A ordered list of entries close to input position and the number of entries per search.</returns>
CloseIdsAndNrOf GetEntries(int32_t  nrPositions, Position* position, float d, int32_t  maxEntities, SpatialHash* spatialHash)
{
    return spatialHash->GetCloseEntriesBulk(nrPositions, position, d, maxEntities);
}

/// <summary>
/// Checks if entries of input spatial hash has changed and updates itself accordingly.
/// </summary>
/// <param name="spatialHash">The Spatial Hash to update.</param>
void Update(SpatialHash* spatialHash)
{
    spatialHash->UpdateTable();
}

void Remove(uint32_t nrOfEntriesToRemove, uint32_t* entryIndices, SpatialHash* spatialHash)
{
    spatialHash->RemoveEntryFromTableBulk(nrOfEntriesToRemove, entryIndices);
}
