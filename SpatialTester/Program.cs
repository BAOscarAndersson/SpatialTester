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

            //Console.WriteLine(ExtGetEnteredSize());
            Console.ReadLine();
            IntPtr intPtr = Marshal.AllocHGlobal((int)(20 * nrEntries));
            //Marshal.Copy(byteArray, 0, intPtr, Marshal.SizeOf(byteArray));

            uint didItRun = Start(nrEntries, intPtr);
            Console.WriteLine("Did it run? (zero is yes): " + didItRun);


            uint didItStop = Stop();
            Console.WriteLine("Did it stop? (zero is yes): " + didItStop);


            Marshal.FreeHGlobal(intPtr);
            Console.WriteLine("Deallocated the array");

            Console.ReadLine();
        }
    }

    //[StructLayout(LayoutKind.Sequential)]
    //public struct Position
    //{
    //    public float x;
    //    public float y;

    //    public Position(uint inX, uint inY)
    //    {
    //        x = inX;
    //        y = inY;
    //    }
    //}

    //[StructLayout(LayoutKind.Sequential)]
    //public struct Entry
    //{
    //    public uint id;
    //    public Position position;

    //    public Entry(uint inId, Position inPos)
    //    {
    //        id = inId;
    //        position = inPos;
    //    }
    //}

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

