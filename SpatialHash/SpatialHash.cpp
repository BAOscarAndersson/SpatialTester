#include "pch.h"
#include "SpatialHash.h"

using namespace std;

// Interop declarations.
extern "C" __declspec(dllexport) void* Start(uint32_t nrEntries, Entered * inEntries, uint32_t tableSize);
extern "C" __declspec(dllexport) uint32_t Stop(SpatialHash * spatialHash);
extern "C" __declspec(dllexport) CloseEntriesAndNrOf GetEntries(const Position position, float d, const unsigned short int maxEntities, SpatialHash * spatialHash);
extern "C" __declspec(dllexport) void Update(uint32_t numberOfEntries, SpatialHash * spatialHash);
extern "C" __declspec(dllexport) void Remove(uint32_t entryIndex, SpatialHash * spatialHash);

// Temporary value that will probably be determined at runtime later.
constexpr uint32_t reservedLocalEntries = 8;

// Contains the offsets. Used to be done manually and it was easier to put them in two different vectors then.
vector<int32_t> xOffsetsToCalculate{};
vector<int32_t> yOffsetsToCalculate{};

// the GetCloseEntries will be looped through in steps of these sizes.
vector<size_t> stepSizes{};

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
SpatialHash::SpatialHash(size_t sideLength) : allEntries(allEntries), sideLength(sideLength), xMask(sideLength - 1), yMask(sideLength - 1)
{
    // Verify that sideLength is a power of two.
  /*  int i = 0;
    bool lengthVerification = false;
    do {

        if (pow(2, i) == sideLength)
            lengthVerification = true;

        if (i > 20)
        {
            cout << "ERROR length of the Spatial Hash needs to be a power of two, greater than 2^0 and less than 2^20" << '\n';
            return;
        }

        i++;

    } while (lengthVerification == false);*/

    // table represents a two dimensional square.
    table = new vector<Cell>();
    table->resize(sideLength * sideLength);

    for (uint32_t i = 0; i != table->size(); i++)
    {
        table->at(i).localEntries = new vector<Entered*>();
        table->at(i).localEntries->reserve(reservedLocalEntries);

        uint32_t numberOfOffsets = 0;

        for (uint32_t j = 0; j != stepSizes.size(); j++)
            numberOfOffsets += stepSizes[j];

        table->at(i).offsets = new vector<uint32_t>();
        table->at(i).offsets->reserve(numberOfOffsets);
    }
    InitializeOffsets();
    closeEntries = new vector<EntryWithDistance>();
    closeEntries->reserve(100);
}

SpatialHash::~SpatialHash()
{
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
    // If the entry is at the end of the vector.
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
/// <returns>Entities within the search area.</returns>
CloseEntriesAndNrOf SpatialHash::GetCloseEntries(const Position pos, const float d, const unsigned short int maxEntities)
{
    // Since new entries to return is to be calculated we need to get rid of the old ones.
    closeEntries->clear();

    // This is the cell that will be the origo of the search.
    uint32_t cellNr = CalculateCellNr(pos);

    // A bunch of loop variables.
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t stepSize = 0;

    /* Loops through the different steps. If you find enough close entities
    in a step, you can end the loop and return since there can be no other closer
    entites. */
    do {
        stepSize += stepSizes.at(i);

        // Loops through all the offsets that belong to the current step.
        while (j < stepSize)
        {
            uint32_t offsetCell = cellNr + table->at(cellNr).offsets->at(j);

            // Loops through all the entries of the current cell.
            for (size_t m = 0; m < table->at(offsetCell).localEntries->size(); m++)
            {
                // Looks crazy, but here we are.
                EntryWithDistance tempEntry = EntryWithDistance(
                    table->at(offsetCell).localEntries->at(m)->entry,
                    Distance(table->at(offsetCell).localEntries->at(m)->entry.position, pos));

                /* Imporant because entites sharing cells can be very far from each other because
                 * of the hashing/modulo on insertion. */
                if (tempEntry.distance < d)
                {
                    closeEntries->push_back(tempEntry);
                }
            }

            /* Insert-sort the vector. The whole point of the Spatial Hash is that there should only be
             * a small number of elements in this list so using Insert Sort probably makes sense.
             * This should be tested sometime though. */
            for (int32_t h = 1; h < static_cast<int>(closeEntries->size()); h++)
            {
                EntryWithDistance tempEntry = closeEntries->at(h);

                int32_t k = h - 1;
                while (k >= 0 && closeEntries->at(k).distance > tempEntry.distance)
                {
                    closeEntries->at(k + 1) = closeEntries->at(k);
                    k--;
                }

                closeEntries->at(k + 1) = tempEntry;
            }

            j++;
        }

        if (closeEntries->size() >= maxEntities)
        {
            closeEntries->resize(maxEntities);
            return CloseEntriesAndNrOf{ static_cast<uint32_t>(closeEntries->size()), closeEntries->data() };
        }

        i++;

    } while (i < stepSizes.size());

    return CloseEntriesAndNrOf{ static_cast<uint32_t>(closeEntries->size()), closeEntries->data() };
}

uint32_t SpatialHash::GetEnteredSize()
{
    return sizeof(Entered);
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
/// It's definitly stupid to recalculate them all the time... It might be that these will be pre-calculated
/// before execution time though.
/// </summary>
void SpatialHash::InitializeOffsets()
{
    for (int32_t y = 0; y != sideLength; y++)
    {
        for (int32_t x = 0; x != sideLength; x++)
        {
            uint32_t i = x + y * sideLength;
            for (uint32_t j = 0; j < xOffsetsToCalculate.size(); j++)
            {
                int32_t tx = x + xOffsetsToCalculate[j];
                int32_t ty = y + yOffsetsToCalculate[j];
                tx = ProperMod(tx, sideLength);
                ty = ProperMod(ty, sideLength);
                table->at(i).offsets->push_back((tx + ty * sideLength) - i);
            }
        }
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
    // The offsets are calculated before runtime and saved in a file that is loaded here.
    ifstream offsetsFile;
    offsetsFile.open("F:\\Prog\\Repos\\SpaceFiller\\bin\\Debug\\netcoreapp3.1\\Offsets.abi", ios::in | ios::binary);

    if (offsetsFile.is_open())
    {
        int32_t numberOfSteps = 0;

        offsetsFile.read((char*)&numberOfSteps, sizeof(numberOfSteps));

        for (uint32_t i = 0; i < numberOfSteps; i++)
        {
            int32_t stepSize = 0;
            offsetsFile.read((char*)&stepSize, sizeof(stepSize));

            stepSizes.push_back(stepSize);

            for (uint32_t j = 0; j < stepSize; j++)
            {
                uint32_t x = 0;
                offsetsFile.read((char*)&x, sizeof(x));

                uint32_t y = 0;
                offsetsFile.read((char*)&y, sizeof(y));

                xOffsetsToCalculate.push_back(x);
                yOffsetsToCalculate.push_back(y);
            }
        }

        offsetsFile.close();
    }

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
CloseEntriesAndNrOf GetEntries(const Position position, float d, const unsigned short int maxEntities, SpatialHash* spatialHash)
{
    return spatialHash->GetCloseEntries(position, d, maxEntities);
}

/// <summary>
/// Checks if entries of input spatial hash has changed and updates itself accordingly.
/// </summary>
/// <param name="numberOfEntries">Number of entries to check if they need to be updated.</param>
/// <param name="spatialHash">The spatial hash to update.</param>
void Update(uint32_t numberOfEntries, SpatialHash* spatialHash)
{
    spatialHash->UpdateTable(numberOfEntries);
}

void Remove(uint32_t entryIndex, SpatialHash* spatialHash)
{
    spatialHash->RemoveEntry(&spatialHash->allEntries[entryIndex]);
}
