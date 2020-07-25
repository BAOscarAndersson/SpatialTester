using System;
using System.Globalization;
using System.Runtime.InteropServices;
using System.Security.Cryptography.X509Certificates;
using System.Threading;

namespace SpatialTester
{
    class Tester
    {
        [DllImport("SpatialHash.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr Start(uint nrEntries, IntPtr inEntries, uint tableSize);

        [DllImport("SpatialHash.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern uint Stop(IntPtr spatialHash);

        [DllImport("SpatialHash.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern CloseEntriesAndNrOf GetEntries(Position position, float d, uint maxEntities, IntPtr spatialHash);

        [DllImport("SpatialHash.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void Update(uint numberOfEntries, IntPtr spatialHash);

        [DllImport("SpatialHash.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void Remove(uint entryIndex, IntPtr spatialHash);

        static void Main(string[] args)
        {
            Tester tester = new Tester();

            uint nrEntries = 1;
            int enteredSize = 20;               // int, float, float, uint, uint = 5*4 = 20byte
            int returnedEntrySize = 16;         // int, float, float, float = 4*4 = 16byte

            IntPtr GlobalEntries = Marshal.AllocHGlobal((int)(enteredSize * nrEntries));

            tester.FillEntries(GlobalEntries, nrEntries, enteredSize);

            try
            {
                IntPtr spatialHash = Start(nrEntries, GlobalEntries, 8);

                // First get.
                Console.WriteLine("1:");
                tester.GetAndPrintCloseEntries(spatialHash, tester, returnedEntrySize);

                // Update.
                tester.ChangeEntriesPosition(GlobalEntries, nrEntries, enteredSize);
                Update(nrEntries, spatialHash);

                // Second get.
                Console.WriteLine("2:");
                tester.GetAndPrintCloseEntries(spatialHash, tester, returnedEntrySize);

                // Removal, danger danger.
                Remove(0, spatialHash);

                // Third get.
                Console.WriteLine("3:");
                tester.GetAndPrintCloseEntries(spatialHash, tester, returnedEntrySize);

                uint didItStop = Stop(spatialHash);
            }
            finally
            {
                Marshal.FreeHGlobal(GlobalEntries);
            }

            Console.ReadLine();
        }

        void GetAndPrintCloseEntries(IntPtr spatialHash, Tester tester, int returnedEntrySize)
        {
            CloseEntriesAndNrOf returnedEntries = new CloseEntriesAndNrOf();
            Position testPosition = new Position { x = 500f, y = 500f };
            returnedEntries = GetEntries(testPosition, 10, 5, spatialHash);

            EntryWithDistance[] entriesOnly = tester.ReadEntries(returnedEntries.allCloseEntries, returnedEntries.nrOfEntries, returnedEntrySize);

            tester.PrintEntriesAndDistances(entriesOnly);
        }

        void FillEntries(IntPtr entries, uint nrEntries, int enteredSize)
        {
            Random rand = new Random();

            for (int i = 0; i < nrEntries; i++)
            {
                float x = 500 + (float)rand.NextDouble()*5;
                float y = 500 + (float)rand.NextDouble()*5;
                uint id = (uint)rand.Next(100, 999);

                // Write id
                Marshal.WriteInt32(entries, i * enteredSize + 0 * 4, (int)id);

                // Write x
                for (int j = 0; j < 4; j++)
                {
                    byte[] xBytes = BitConverter.GetBytes(x);
                    Marshal.WriteByte(entries, i * enteredSize + 1*4 + j, xBytes[j]);
                }

                // Write y
                for (int j = 0; j < 4; j++)
                {
                    byte[] yBytes = BitConverter.GetBytes(y);
                    Marshal.WriteByte(entries, i * enteredSize + 2*4 + j, yBytes[j]);
                }

                // Write zero to these, because it's internal stuff for the Spatial Hash.
                Marshal.WriteInt32(entries, i * enteredSize + 3 * 4, 0);
                Marshal.WriteInt32(entries, i * enteredSize + 4 * 4, 0);
            }
        }

        void ChangeEntriesPosition(IntPtr entries, uint nrEntries, int enteredSize)
        {
            byte[] xBytes = new byte[4];
            byte[] yBytes = new byte[4];

            for (int i = 0; i < nrEntries; i++)
            {
                // Read and change X
                for (int j = 0; j < 4; j++)
                {
                    xBytes[j] = Marshal.ReadByte(entries, i * enteredSize + 1 * 4 + j);
                }
                float x = BitConverter.ToSingle(xBytes, 0);

                x += 0.01f;

                for (int j = 0; j < 4; j++)
                {
                    xBytes = BitConverter.GetBytes(x);
                    Marshal.WriteByte(entries, i * enteredSize + 1 * 4 + j, xBytes[j]);
                }

                // Read and change Y
                for (int j = 0; j < 4; j++)
                {
                    yBytes[j] = Marshal.ReadByte(entries, i * enteredSize + 2 * 4 + j);
                }
                float y = BitConverter.ToSingle(xBytes, 0);

                y += 0.01f;

                for (int j = 0; j < 4; j++)
                {
                    yBytes = BitConverter.GetBytes(y);
                    Marshal.WriteByte(entries, i * enteredSize + 2 * 4 + j, yBytes[j]);
                }
            }
        }

       EntryWithDistance[] ReadEntries(IntPtr entries, uint nrEntries, int entrySize)
        {
            EntryWithDistance[] returnedEntries = new EntryWithDistance[nrEntries];
            byte[] tempBytes = new byte[4];

            for (int i = 0; i < nrEntries; i++)
            {
                // Read id
                for (int j = 0; j < 4; j++)
                {
                    tempBytes[j] = Marshal.ReadByte(entries, i * entrySize + 0 * 4 + j);
                }
                returnedEntries[i].entry.id = BitConverter.ToUInt32(tempBytes, 0);

                // Read x
                for (int j = 0; j < 4; j++)
                {
                    tempBytes[j] = Marshal.ReadByte(entries, i * entrySize + 1 * 4 + j);
                }
                returnedEntries[i].entry.position.x = BitConverter.ToSingle(tempBytes, 0);

                // Read y
                for (int j = 0; j < 4; j++)
                {
                    tempBytes[j] = Marshal.ReadByte(entries, i * entrySize + 2 * 4 + j);
                }
                returnedEntries[i].entry.position.y = BitConverter.ToSingle(tempBytes, 0);

                // Read distance from test position.
                for (int j = 0; j < 4; j++)
                {
                    tempBytes[j] = Marshal.ReadByte(entries, i * entrySize + 3 * 4 + j);
                }
                returnedEntries[i].distance = BitConverter.ToSingle(tempBytes, 0);
            }

            return returnedEntries;
        }

        void PrintEntriesAndDistances(EntryWithDistance[] returnedEntries)
        {
            foreach(EntryWithDistance aEntry in returnedEntries)
            {
                Console.WriteLine("id: " + aEntry.entry.id + " x: " + aEntry.entry.position.x + " y: " + aEntry.entry.position.y + " dist: " + aEntry.distance);
            }
        }


    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Position
    {
        public float x;
        public float y;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Entry
    {
        public uint id;
        public Position position;
    }

    // int, float, float, uint, uint = 5*4 = 20byte
    [StructLayout(LayoutKind.Sequential)]
    public struct Entered
    {
        public Entry entry;
        public uint nrInCell;
        public uint hashValue;
    }

    // int, float, float, float = 4*4 = 16byte
    [StructLayout(LayoutKind.Sequential)]
    struct EntryWithDistance
    {
        public Entry entry;
        public float distance;
    };

    struct CloseEntriesAndNrOf
    {
        public uint nrOfEntries;
        public IntPtr allCloseEntries;
    };
}

