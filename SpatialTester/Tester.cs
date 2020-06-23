using System;
using System.Runtime.InteropServices;
using System.Security.Cryptography.X509Certificates;
using System.Threading;

namespace SpatialTester
{
    class Tester
    {
        [DllImport("SpatialHash.dll")]
        public static extern uint Start(uint nrEntries, IntPtr inEntries);

        [DllImport("SpatialHash.dll")]
        public static extern uint Stop();

        [DllImport("SpatialHash.dll")]
        public static extern uint ExtGetEnteredSize();

        static void Main(string[] args)
        {
            Tester tester = new Tester();

            uint nrEntries = 100;

            //Console.WriteLine(ExtGetEnteredSize());

            IntPtr GlobalEntries = Marshal.AllocHGlobal((int)(20 * nrEntries));
            //Marshal.Copy(byteArray, 0, intPtr, Marshal.SizeOf(byteArray));

            tester.FillEntries(GlobalEntries, nrEntries);

            try
            {
                uint didItRun = Start(nrEntries, GlobalEntries);

                uint didItStop = Stop();
            }
            finally
            {
                Marshal.FreeHGlobal(GlobalEntries);
            }

            Console.ReadLine();
        }

        void FillEntries(IntPtr entries, uint nrEntries)
        {
            Random rand = new Random();

            //rand.Next(100, 999);
            //(float)rand.NextDouble() * 5 + 500;

            Entered[] entered = new Entered[nrEntries];

            for (uint i = 0; i <= nrEntries; i++)
            {
                Position tempPos = new Position() { x = (float)rand.NextDouble() * 5 + 500, y = (float)rand.NextDouble() * 5 + 500};
                Entry aEntry = new Entry() { id = (uint)rand.Next(100, 999), position = tempPos };
                Entered aEntered = new Entered() { entry = aEntry, nrInCell = 0, hashValue = 0 };
                entered[i] = aEntered;
            }

            Marshal.Copy(entered, 0, entries, nrEntries);
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

    [StructLayout(LayoutKind.Sequential)]
    public struct Entered
    {
        public Entry entry;
        public uint nrInCell;
        public uint hashValue;
    }

    [StructLayout(LayoutKind.Sequential)]
    struct EntryWithDistance
    {
        public Entry entry;
        public float distance;
    };


    //[StructLayout(LayoutKind.Sequential)]
    //public struct EntryWithDistance
    //{
    //    public Entry entry;
    //    public float distance;
    //}
}

