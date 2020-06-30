using System;
using System.Globalization;
using System.Runtime.InteropServices;
using System.Security.Cryptography.X509Certificates;
using System.Threading;

namespace SpatialTester
{
    class Tester
    {
        [DllImport("SpatialHash.dll")]
        public static extern IntPtr Start(uint nrEntries, IntPtr inEntries);

        [DllImport("SpatialHash.dll")]
        public static extern uint Stop(IntPtr spatialHash);

        [DllImport("SpatialHash.dll")]
        public static extern CloseEntriesAndNrOf ExtGetCloseEntries(Position position, float d, uint maxEntities, IntPtr spatialHash);

        static void Main(string[] args)
        {
            Tester tester = new Tester();

            uint nrEntries = 1000;
            int enteredSize = 20;               // int, float, float, uint, uint = 5*4 = 20byte
            int returnedEntrySize = 16;         // int, float, float, float = 4*4 = 16byte

            IntPtr GlobalEntries = Marshal.AllocHGlobal((int)(enteredSize * nrEntries));

            tester.FillEntries(GlobalEntries, nrEntries, enteredSize);

            try
            {
                IntPtr spatialHash = Start(nrEntries, GlobalEntries);

                CloseEntriesAndNrOf returnedEntries = new CloseEntriesAndNrOf();

                Position testPosition = new Position{x = 500f, y = 500f};
                returnedEntries = ExtGetCloseEntries(testPosition, 10, 5, spatialHash);

                EntryWithDistance[] entriesOnly = tester.ReadEntries(returnedEntries.allCloseEntries, returnedEntries.nrOfEntries, returnedEntrySize);

                tester.PrintEntriesAndDistances(entriesOnly);

                uint didItStop = Stop(spatialHash);
            }
            finally
            {
                Marshal.FreeHGlobal(GlobalEntries);
            }

            Console.ReadLine();
        }

        void FillEntries(IntPtr entries, uint nrEntries, int enteredSize)
        {
            Random rand = new Random();

            //Console.WriteLine("C# gives:");
            for (int i = 0; i < nrEntries; i++)
            {
                float x = 500 + (float)rand.NextDouble()*5;
                float y = 500 + (float)rand.NextDouble()*5;
                uint id = (uint)rand.Next(100, 999);

                //Console.WriteLine("id:" + id + ", x: " + x + ", y: " + y);

                Marshal.WriteInt32(entries, i * enteredSize + 0 * 4, (int)id);
                for (int j = 0; j < 4; j++)
                {
                    byte[] xBytes = BitConverter.GetBytes(x);
                    Marshal.WriteByte(entries, i * enteredSize + 1*4 + j, xBytes[j]);
                }
                for (int j = 0; j < 4; j++)
                {
                    byte[] yBytes = BitConverter.GetBytes(y);
                    Marshal.WriteByte(entries, i * enteredSize + 2*4 + j, yBytes[j]);
                }
                Marshal.WriteInt32(entries, i * enteredSize + 3 * 4, 100110);
                Marshal.WriteInt32(entries, i * enteredSize + 4 * 4, 7999);
            }
        }

        EntryWithDistance[] ReadEntries(IntPtr entries, uint nrEntries, int entrySize)
        {
            EntryWithDistance[] returnedEntries = new EntryWithDistance[nrEntries];
            byte[] tempBytes = new byte[4];

            for (int i = 0; i < nrEntries; i++)
            {
                for (int j = 0; j < 4; j++)
                {
                    tempBytes[j] = Marshal.ReadByte(entries, i * entrySize + 0 * 4 + j);
                }
                returnedEntries[i].entry.id = BitConverter.ToUInt32(tempBytes, 0);
                for (int j = 0; j < 4; j++)
                {
                    tempBytes[j] = Marshal.ReadByte(entries, i * entrySize + 1 * 4 + j);
                }
                returnedEntries[i].entry.position.x = BitConverter.ToSingle(tempBytes, 0);
                for (int j = 0; j < 4; j++)
                {
                    tempBytes[j] = Marshal.ReadByte(entries, i * entrySize + 2 * 4 + j);
                }
                returnedEntries[i].entry.position.y = BitConverter.ToSingle(tempBytes, 0);
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

