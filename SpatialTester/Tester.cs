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
        public static extern uint Start(uint nrEntries, IntPtr inEntries);

        [DllImport("SpatialHash.dll")]
        public static extern uint Stop();

        [DllImport("SpatialHash.dll")]
        public static extern uint ExtGetEnteredSize();

        static void Main(string[] args)
        {
            Tester tester = new Tester();

            uint nrEntries = 10;
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

            Console.WriteLine("C# gives:");
            for (int i = 0; i <= nrEntries; i++)
            {
                float x = (float)rand.NextDouble() * 500;
                float y = (float)rand.NextDouble() * 500;
                uint id = (uint)rand.Next(100, 999);

                Console.WriteLine("id:" + id + ", x: " + x + ", y: " + y);

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

