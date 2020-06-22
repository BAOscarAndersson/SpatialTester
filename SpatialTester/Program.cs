using System;
using System.Runtime.InteropServices;
using System.Security.Cryptography.X509Certificates;
using System.Threading;

namespace SpatialTester
{
    class Program
    {
        [DllImport("SpatialHash.dll")]
        public static extern uint Start(uint nrEntries, IntPtr inEntries);

        [DllImport("SpatialHash.dll")]
        public static extern uint Stop();

        [DllImport("SpatialHash.dll")]
        public static extern uint ExtGetEnteredSize();

        static void Main(string[] args)
        {

            uint nrEntries = 100;

            Random rand = new Random();

            //rand.Next(100, 999);
            //(float)rand.NextDouble() * 5 + 500;

            //Console.WriteLine(ExtGetEnteredSize());

            IntPtr intPtr = Marshal.AllocHGlobal((int)(20 * nrEntries));
            //Marshal.Copy(byteArray, 0, intPtr, Marshal.SizeOf(byteArray));

            try
            {
                uint didItRun = Start(nrEntries, intPtr);

                uint didItStop = Stop();
            }
            finally
            {
                Marshal.FreeHGlobal(intPtr);
            }

            Console.ReadLine();
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Position
    {
        public float x;
        public float y;

        public Position(uint inX, uint inY)
        {
            x = inX;
            y = inY;
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Entry
    {
        public uint id;
        public Position position;

        public Entry(uint inId, Position inPos)
        {
            id = inId;
            position = inPos;
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    struct EntryWithDistance
    {
        Entry entry;
        float distance;
    };


    //[StructLayout(LayoutKind.Sequential)]
    //public struct EntryWithDistance
    //{
    //    public Entry entry;
    //    public float distance;

    //    public EntryWithDistance(Entry inEntry, float inDistance)
    //    {
    //        entry = inEntry;
    //        distance = inDistance;
    //    }
    //}
}

