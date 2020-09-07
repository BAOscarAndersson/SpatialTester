#include "pch.h"
#include "SpatialHash.h"

using namespace std;

// Interop declarations.
extern "C" __declspec(dllexport) void* Start(uint32_t nrEntries, Entered * inEntries, uint32_t tableSize);
extern "C" __declspec(dllexport) uint32_t Stop(SpatialHash * spatialHash);
extern "C" __declspec(dllexport) CloseEntriesAndNrOf GetEntries(int32_t nrOfPositions, Position* position, float d, int32_t maxEntities, SpatialHash * spatialHash);
extern "C" __declspec(dllexport) void Update(int32_t numberOfEntries, SpatialHash * spatialHash);
extern "C" __declspec(dllexport) void Remove(uint32_t entryIndex, SpatialHash * spatialHash);

/// <summary>
/// Proper modulo function.
/// </summary>
int ProperMod(const uint32_t a, const int b)
{
    return (a < 0 ? (((a % b) + b) % b) : (a % b));
}

/// <summary>
/// Euclidian distance.
/// </summary>
float Distance(const Position a, const Position b)
{
    return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));
}

/// <summary>
/// Creates a Spatial Hash of a certain size.
/// </summary>
/// <param name="sideLength">The size of the Spatial Hash, needs to be a power of two.</param>
SpatialHash::SpatialHash(size_t sidePower) : allEntries(allEntries), sideLength(pow(2, sidePower)+1), xMask(sideLength-1), yMask(sideLength-1)
{
    // Temporary value that will probably be determined at runtime later.
    constexpr uint32_t reservedLocalEntries = 16;

    // table represents a two dimensional square.
    table = new vector<Cell>();
    table->resize(sideLength * sideLength);

    for (uint32_t i = 0; i != table->size(); i++)
    {
        table->at(i).localEntries = new vector<Entered*>();
        table->at(i).localEntries->reserve(reservedLocalEntries);

        table->at(i).offsets = new vector<vector<uint32_t>*>();

        globalOffsets = new vector<vector<uint32_t>*>();
        globalOffsets->reserve(100);
    }
    
    closeEntries = new vector<EntryWithDistance>();
    nrOfEntries = new vector<uint32_t>();
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
void SpatialHash::Initilize(Entered* inAllEntries, uint32_t numberOfEntries)
{
    InitializeOffsets();

    allEntries = inAllEntries;

    for (uint32_t i = 0; i < numberOfEntries; i++)
    {
        InsertInTable(&allEntries[i]);
    }
}

/// <summary>
/// Checks if any entry in allEntries have moved to another cell,
/// in which case it inserts them into that cell and removes them
/// from their old one.
/// </summary>
void SpatialHash::UpdateTable(uint32_t numberOfEntries)
{
    for (uint32_t i = 0; i < numberOfEntries; i++)
    {
        UpdateEntry(&allEntries[i]);
    }
}

/// <summary>
/// Removes input entry from hash table.
/// </summary>
/// <param name="entry">Entry to remove from hash table.</param>
void SpatialHash::RemoveEntry(Entered* entry)
{
    // If the entry is at the end of the vector it can be removed straight away.
    if (entry->nrInCell == table->at(entry->hashValue).localEntries->size() - 1)
    {
        table->at(entry->hashValue).localEntries->pop_back();
    }
    else
    {
        // If the entry isn't at the end of the vector, it's switched with the entry that is.
        table->at(entry->hashValue).localEntries->back()->nrInCell = entry->nrInCell;
        table->at(entry->hashValue).localEntries->at(entry->nrInCell) = table->at(entry->hashValue).localEntries->back();
        table->at(entry->hashValue).localEntries->pop_back();
    }
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
        for(uint32_t j = 0; j< table->at(cellNr).offsets->at(i)->size(); j++)
        {

            uint32_t offsetCell = cellNr + table->at(cellNr).offsets->at(i)->at(j);

            // Loops through all the entries of the current cell.
            for (size_t m = 0; m < table->at(offsetCell).localEntries->size(); m++)
            {
                float tempDistance = Distance(table->at(offsetCell).localEntries->at(m)->entry.position, pos);

                /* Imporant because entites sharing cells can be very far from each other because
                 * of the hashing/modulo on insertion. */
                if (tempDistance < d)
                {
                    Entry tempEntry = table->at(offsetCell).localEntries->at(m)->entry;
                    EntryWithDistance tempEntryWithDistance = EntryWithDistance(tempEntry, tempDistance);

                    closeEntries->push_back(tempEntryWithDistance);
                }
            }
        }

        // If there are to many entries only the closest should be kept so they need to be orderd.
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
/// Sorts the elements of closeEntries which is needed for GetCloseEntries
/// to be able to tell which elements are the closests.
/// </summary>
/// <param name="from">From which index to sort the list.</param>
void SpatialHash::SortCloseEntries(int32_t from)
{
    /* Insert-sort the vector. The whole point of the Spatial Hash is that there should only be
     * a small number of elements in this list so using Insert Sort probably makes sense.
     * This should be tested sometime though. */
    for (int32_t h = from; h < static_cast<int>(closeEntries->size()); h++)
    {
        EntryWithDistance tempEntry = closeEntries->at(h);

        int32_t k = h - 1;
        while (k >= from && closeEntries->at(k).distance > tempEntry.distance)
        {
            closeEntries->at(k + 1) = closeEntries->at(k);
            k--;
        }

        closeEntries->at(k + 1) = tempEntry;
    }

    /* Selection sort*/
    /*for (int32_t i = from; i < closeEntries->size() - 1; i++)
    {
        int jMin = i;

        for (int32_t j = i + 1; j < closeEntries->size(); j++)
        {
            if (closeEntries->at(j).distance < closeEntries->at(jMin).distance)
            {
                jMin = j;
            }
        }

        if (jMin != i)
        {
            swap(closeEntries->at(i), closeEntries->at(jMin));
        }
    }*/
}

CloseEntriesAndNrOf SpatialHash::GetCloseEntriesBulk(int32_t nrSearches, Position* pos, float d, int32_t maxEntities)
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

    return CloseEntriesAndNrOf{ nrOfEntries->data(), closeEntries->data() };
}

/// <summary>
/// Inserts an entry into the spatial hash table.
/// </summary>
/// <param name="entry">Entry to insert.</param>
void SpatialHash::InsertInTable(Entered* entered)
{
    uint32_t cellNr = CalculateCellNr(entered->entry.position);
    SpatialHash::InsertInTable(entered, cellNr);
}
void SpatialHash::InsertInTable(Entered* entered, uint32_t cellNr)
{
    // The index for the entry in localEntries and in table is saved for efficient removal.
    entered->nrInCell = this->table->at(cellNr).localEntries->size();
    entered->hashValue = cellNr;

    this->table->at(cellNr).localEntries->push_back(entered);
}

/// <summary>
/// The hashing function. Calculates where in the spatial hash a position ends up.
/// </summary>
/// <param name="pos">The position to find a cell for.</param>
/// <returns>The cell number associated with the input position.</returns>
unsigned int SpatialHash::CalculateCellNr(Position pos)
{
    return CalculateCellNr(pos.x, pos.y);
}

/// <summary>
/// The hashing function. Calculates where in the spatial hash a position ends up.
/// </summary>
/// <param name="x">Horizontal position to find a cell for.</param>
/// <param name="y">Vertical position to find a cell for.</param>
/// <returns>The cell number associated with the input position.</returns>
unsigned int SpatialHash::CalculateCellNr(float x, float y)
{
    // Calculate where in the theoretical 2d hash table the entry would end up.
    unsigned int xCellNr = static_cast<unsigned int>(x) & xMask;
    unsigned int yCellNr = static_cast<unsigned int>(y) & yMask;

    // Convert this to the actual cell in the vector.
    return xCellNr + yCellNr * sideLength;
}

/// <summary>
/// Currently each cell gets its own offset calculated. Since most cells share many offsets,
/// it's a bit of a vaste memory wise. On the other hand it might be better performance wise from a time perspective.
/// </summary>
void SpatialHash::InitializeOffsets()
{
    ReadOffsetsFromFile();

    numberOfOffsets = 0;
    for (uint32_t j = 0; j < xOffsetsToCalculate.size(); j++)
        numberOfOffsets += xOffsetsToCalculate[j].size();

    for (int32_t y = 0; y != sideLength; y++)
    {
        for (int32_t x = 0; x != sideLength; x++)
        {
            InitializeOffsetsInCell(x, y);
        }
    }
}

void SpatialHash::InitializeOffsetsInCell(uint32_t x, uint32_t y)
{
    uint32_t i = x + y * sideLength;

    vector<uint32_t>* cellOffsets;

    // Loop through all the steps.
    for (uint32_t k = 0; k < numberOfSteps; k++)
    {
        cellOffsets = new vector<uint32_t>();

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

void SpatialHash::ReadOffsetsFromFile()
{
    // The offsets are calculated before runtime and saved in a file that is loaded here.
    ifstream offsetsFile;
    offsetsFile.open("F:\\Prog\\Repos\\SpaceFiller\\bin\\Debug\\netcoreapp3.1\\Offsets.abi", ios::in | ios::binary);

    if (offsetsFile.is_open())
    {
        numberOfSteps = 0;

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

/// <summary>
/// Checks if inputed entry have moved to a new cell. If it has
/// moved, it is reinserted and the old one is removed.
/// </summary>
/// <param name="entry">Entry will be put into correct cell.</param>
void SpatialHash::UpdateEntry(Entered* entry)
{
    uint32_t currenthashValue = CalculateCellNr(entry->entry.position);
    if (currenthashValue != entry->hashValue)
    {
        RemoveEntry(entry);
        InsertInTable(entry, currenthashValue);
    }
}

/// <summary>
/// Interop for constructing the SpatialHash class and initializing it.
/// </summary>
/// <param name="nrEntries">The amount of entries to start the hash with, allocating and such.</param>
/// <param name="globalEntries">The array of structres that represents something put into the Spatial Hash.</param>
/// <param name="tableSize">The length of a side of the square Spatial Hash, needs to be 2^n. (Where n is a integer > 0.)</param>
/// <returns>A pointer to the started SpatialHash.</returns>
void* Start(uint32_t nrEntries, Entered* globalEntries, uint32_t tableSize)
{
    SpatialHash* spatialHash = new SpatialHash(tableSize);
    spatialHash->Initilize(globalEntries, nrEntries);

    return spatialHash;
}

/// <summary>
/// Have to be called or may the Theoretical Gods have mercy on my soul.
/// </summary>
/// <returns>0 if it ran to completion.</returns>
uint32_t Stop(SpatialHash* spatialHash)
{
    delete spatialHash;

    return 0;
}

/// <summary>
/// Retrives entries close to input position from the spatialHash.
/// </summary>
/// <param name="position">Position to check for close entries.</param>
/// <param name="d">How far away from the position entries can be to be close.</param>
/// <param name="maxEntities">Maximum number of entries to return.</param>
/// <param name="spatialHash">Which spatialHash to look in.</param>
/// <returns>A ordered list of entries close to input position.</returns>
CloseEntriesAndNrOf GetEntries(int32_t  nrPositions, Position* position, float d, int32_t  maxEntities, SpatialHash* spatialHash)
{
    return spatialHash->GetCloseEntriesBulk(nrPositions, position, d, maxEntities);
}

/// <summary>
/// Checks if entries of input spatial hash has changed and updates itself accordingly.
/// </summary>
/// <param name="numberOfEntries">Number of entries to check if they need to be updated.</param>
/// <param name="spatialHash">The spatial hash to update.</param>
void Update(int32_t numberOfEntries, SpatialHash* spatialHash)
{
    spatialHash->UpdateTable(numberOfEntries);
}

void Remove(uint32_t entryIndex, SpatialHash* spatialHash)
{
    spatialHash->RemoveEntry(&spatialHash->allEntries[entryIndex]);
}
