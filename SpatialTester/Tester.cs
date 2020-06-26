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
            int enteredSize = 20;
            //Console.WriteLine(ExtGetEnteredSize());

            IntPtr GlobalEntries = Marshal.AllocHGlobal((int)(enteredSize * nrEntries));
            //Marshal.Copy(byteArray, 0, intPtr, Marshal.SizeOf(byteArray));

            tester.FillEntries(GlobalEntries, nrEntries, enteredSize);

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

        void FillEntries(IntPtr entries, uint nrEntries, int enteredSize)
        {
            Random rand = new Random();

            //rand.Next(100, 999);
            //(float)rand.NextDouble() * 5 + 500;


            for (int i = 0; i <= nrEntries; i++)
            {
                float x = (float)rand.NextDouble() * 5 + 500;
                float y = (float)rand.NextDouble() * 5 + 500;
                uint id = (uint)rand.Next(100, 999);

                Marshal.WriteInt32(entries, i * enteredSize, (int)x);
                Marshal.WriteInt32(entries, i * enteredSize, (int)y);
                Marshal.WriteInt32(entries, i * enteredSize, (int)id);
                Marshal.WriteInt32(entries, i * enteredSize, 0);
                Marshal.WriteInt32(entries, i * enteredSize, 0);
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

