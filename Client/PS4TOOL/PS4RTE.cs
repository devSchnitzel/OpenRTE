using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace PS4TOOL
{
    public class PS4RTE
    {
        private Socket ps4;
        private Object ps4_lock = new Object();

        [StructLayout(LayoutKind.Sequential), Serializable]
        public struct Process
        {
            public int process_id;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 50)]
            public string process_name;
        }

        [StructLayout(LayoutKind.Sequential), Serializable]
        public struct Page
        {
            public ulong start;
            public ulong stop;
            public int protection;
        }

        [StructLayout(LayoutKind.Sequential), Serializable]
        public struct GetPageRequest
        {
            public int process_id;
        }

        [StructLayout(LayoutKind.Sequential), Serializable]
        public struct GetMemoryRequest
        {
            public int process_id;
            public ulong offset;
            public int size;
        }

        [StructLayout(LayoutKind.Sequential), Serializable]
        public struct SetMemoryRequest
        {
            public int process_id;
            public ulong offset;
            public int size;
        }

        [StructLayout(LayoutKind.Sequential), Serializable]
        public struct ModuleRequest
        {
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
            public string module_name;
        }

        [StructLayout(LayoutKind.Sequential), Serializable]
        public struct NotifyRequest
        {
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
            public string message;
        }

        [StructLayout(LayoutKind.Sequential), Serializable]
        public struct ReturnResponce
        {
            public int ret;
        }

        [StructLayout(LayoutKind.Sequential), Serializable]
        public struct ModuleIDRequest
        {
            public int id;
        }

        [StructLayout(LayoutKind.Sequential), Serializable]
        public struct Module
        {
            public int id;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
            public string name;
            public ulong base_addr;
            public int size;
        }

        [StructLayout(LayoutKind.Sequential), Serializable]
        public struct ResolveRequest
        {
            public int module_id;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
            public string func_name;
        }

        [StructLayout(LayoutKind.Sequential), Serializable]
        public struct UlongResponse
        {
            public ulong ret;
        }

        [StructLayout(LayoutKind.Sequential), Serializable]
        public struct Header
        {
            public int magic;
            public int size;
        }

        [Flags]
        public enum PermFlags
        {
            READ = 0x00000001,
            WRITE = 0x00000002,
            EXEC = 0x00000004
        }

        public enum LED_COLOR
        {
            RED,
            ORANGE,
            BLUE,
            WHITE
        }

        public int transfert_rate = 20000;
        public int recv_prec = 0;

        public PS4RTE()
        {
            ps4 = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
        }

        private static System.Array ResizeArray(System.Array oldArray, int newSize)
        {
            int oldSize = oldArray.Length;
            System.Type elementType = oldArray.GetType().GetElementType();
            System.Array newArray = System.Array.CreateInstance(elementType, newSize);
            int preserveLength = System.Math.Min(oldSize, newSize);
            if (preserveLength > 0)
                System.Array.Copy(oldArray, newArray, preserveLength);
            return newArray;
        }

        private byte[] recv_all()
        {
            recv_prec = 0;
            ps4.ReceiveTimeout = 20000;

            byte[] header_b = new byte[8];
            Console.WriteLine("Receving header ...");
            ps4.Receive(header_b, Marshal.SizeOf(typeof(Header)), 0);

            IntPtr ptr = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(Header)));
            Marshal.Copy(header_b, 0, ptr, Marshal.SizeOf(typeof(Header)));
            Header h = (Header)Marshal.PtrToStructure(ptr, typeof(Header));

            Console.WriteLine(BitConverter.ToString(header_b));

            if (h.magic == 1337)
            {
                int data_size = h.size;
                byte[] data = new byte[0];

                Console.WriteLine(data_size.ToString() + " bytes needed");
                int packets_nbr = (int)Math.Ceiling((decimal)data_size / transfert_rate);

                while (data.Length < data_size)
                {
                    int b_s = transfert_rate;
                    if ((data_size - data.Length) < transfert_rate)
                    {
                        b_s = data_size - data.Length;
                    }

                    Console.WriteLine("Packet: " + b_s.ToString() + " bytes");

                    byte[] buffer = new byte[b_s];
                    int readed = ps4.Receive(buffer);

                    int old_lenght = data.Length;
                    data = (byte[])ResizeArray(data, old_lenght + readed);
                    Array.Copy(buffer, 0, data, old_lenght, readed);

                    int current_packet = data.Length / transfert_rate;
                    recv_prec = current_packet * 100 / packets_nbr;
                }

                Console.WriteLine(data.Length + " bytes recevied");

                return data;
            } else
            {
                Console.WriteLine("Magic code doesn't equal");
            }

            return new byte[] {};
        }

        private int send_all(byte[] data)
        {
            Console.WriteLine("Sending header ...");

            Header h;
            h.magic = 1337;
            h.size = data.Length;

            byte[] request = new byte[Marshal.SizeOf(typeof(Header))];
            IntPtr ptr0 = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(Header)));
            Marshal.StructureToPtr(h, ptr0, true);
            Marshal.Copy(ptr0, request, 0, Marshal.SizeOf(typeof(Header)));
            Marshal.FreeHGlobal(ptr0);

            ps4.Send(request);

            Console.WriteLine("Sending data ...");
            ps4.Send(data);

            Console.WriteLine(data.Length.ToString() + " bytes of data sended !");

            return 0;
        }

        public string GetPerm(int flags)
        {
            string r = "";
            bool is_read = (flags & (int)PermFlags.READ) != 0;
            bool is_write = (flags & (int)PermFlags.WRITE) != 0;
            bool is_exec = (flags & (int)PermFlags.EXEC) != 0;

            if (is_read)
            {
                r += "R";
            }
            else
            {
                r += "-";
            }


            if (is_write)
            {
                r += "W";
            }
            else
            {
                r += "-";
            }

            if (is_exec)
            {
                r += "X";
            }
            else
            {
                r += "-";
            }

            return r;
        }

        public bool Connect(String ip)
        {
            try
            {
                ps4 = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
                ps4.Connect(ip, 13003);
                return true;
            }
            catch
            {
                return false;
            }
        }

        public Process[] GetProcesses()
        {
            Process[] proc = new Process[0];

            lock (ps4_lock)
            {
                send_all(new byte[] { 0x1 });

                byte[] data = recv_all();

                int proc_num = data.Length / Marshal.SizeOf(typeof(Process));

                proc = new Process[proc_num];
                for (int i = 0; i < proc_num; i++)
                {
                    IntPtr ptr = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(Process)));
                    Marshal.Copy(data, (i * Marshal.SizeOf(typeof(Process))), ptr, Marshal.SizeOf(typeof(Process)));

                    Process p = (Process)Marshal.PtrToStructure(ptr, typeof(Process));
                    proc[i] = p;

                    Marshal.FreeHGlobal(ptr);
                }
            }

            return proc;
        }

        public Page[] GetPages(int process_id)
        {

            Page[] pages = new Page[0];

            lock (ps4_lock)
            {
                GetPageRequest p_req;
                p_req.process_id = process_id;

                byte[] request = new byte[1 + Marshal.SizeOf(typeof(GetPageRequest))];
                request[0] = 0x2;

                IntPtr ptr0 = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(GetPageRequest)));
                Marshal.StructureToPtr(p_req, ptr0, true);
                Marshal.Copy(ptr0, request, 1, Marshal.SizeOf(typeof(GetPageRequest)));
                Marshal.FreeHGlobal(ptr0);

                send_all(request);

                byte[] data = recv_all();

                Console.Write("Recv(): " + data.Length.ToString() + " bytes");

                int page_num = data.Length / Marshal.SizeOf(typeof(Page));

                pages = new Page[page_num];
                for (int i = 0; i < page_num; i++)
                {
                    IntPtr ptr = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(Page)));
                    Marshal.Copy(data, (i * Marshal.SizeOf(typeof(Page))), ptr, Marshal.SizeOf(typeof(Page)));

                    Page p = (Page)Marshal.PtrToStructure(ptr, typeof(Page));
                    pages[i] = p;

                    Marshal.FreeHGlobal(ptr);
                }
            }
            return pages;
        }

        public byte[] GetMemory(int process_id, ulong offset, int size)
        {
            byte[] data = new byte[] { };

            lock (ps4_lock)
            {
                GetMemoryRequest g_req;
                g_req.process_id = process_id;
                g_req.offset = offset;
                g_req.size = size;

                byte[] request = new byte[1 + Marshal.SizeOf(typeof(GetMemoryRequest))];
                request[0] = 0x3;

                IntPtr ptr0 = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(GetMemoryRequest)));
                Marshal.StructureToPtr(g_req, ptr0, true);
                Marshal.Copy(ptr0, request, 1, Marshal.SizeOf(typeof(GetMemoryRequest)));
                Marshal.FreeHGlobal(ptr0);

                send_all(request);

                data = recv_all();
            }

            return data;
        }

        public void SetMemory(int process_id, ulong offset, byte[] w_data)
        {
            lock (ps4_lock)
            {
                Console.Write("SetMemory need (0x" + offset.ToString("X"));
                GetMemoryRequest g_req;
                g_req.process_id = process_id;
                g_req.offset = offset;
                g_req.size = w_data.Length;

                byte[] request = new byte[1 + Marshal.SizeOf(typeof(SetMemoryRequest)) + w_data.Length];
                request[0] = 0x4;

                IntPtr ptr0 = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(SetMemoryRequest)));
                Marshal.StructureToPtr(g_req, ptr0, true);
                Marshal.Copy(ptr0, request, 1, Marshal.SizeOf(typeof(SetMemoryRequest)));
                Marshal.FreeHGlobal(ptr0);

                Array.Copy(w_data, 0, request, 1 + Marshal.SizeOf(typeof(SetMemoryRequest)), w_data.Length);

                send_all(request);
                ps4.Receive(new byte[] { 0x00 });
            }
        }

        // à faire
        public void SendNotify(string message)
        {
            lock (ps4_lock)
            {
                Console.Write("Envoie d'une requête de notification ..\n");
                NotifyRequest n_req;
                n_req.message = message;

                byte[] request = new byte[1 + Marshal.SizeOf(typeof(NotifyRequest))];
                request[0] = 0x06;

                IntPtr ptr0 = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(NotifyRequest)));
                Marshal.StructureToPtr(n_req, ptr0, true);
                Marshal.Copy(ptr0, request, 1, Marshal.SizeOf(typeof(NotifyRequest)));
                Marshal.FreeHGlobal(ptr0);

                send_all(request);
            }
        }

        public void Exit()
        {
            lock (ps4_lock)
            {
                send_all(new byte[] { 0x05 });
            }
        }

        public byte[] DumpModule(String module_name)
        {
            byte[] data = new byte[] { };

            lock (ps4_lock)
            {
                Console.Write("Envoie d'une requête de lancement d'un module ..\n");
                ModuleRequest d_req;
                d_req.module_name = module_name;

                byte[] request = new byte[1 + Marshal.SizeOf(typeof(ModuleRequest))];
                request[0] = 0x8;

                IntPtr ptr0 = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(ModuleRequest)));
                Marshal.StructureToPtr(d_req, ptr0, true);
                Marshal.Copy(ptr0, request, 1, Marshal.SizeOf(typeof(ModuleRequest)));
                Marshal.FreeHGlobal(ptr0);

                send_all(request);

                Console.Write("En attente d'une réponse\n");
                data = recv_all();
            }

            return data;
        }

        public int LoadModule(String module_name)
        {
            int ret = -1;

            lock (ps4_lock)
            {
                Console.Write("Envoie d'une requête de load ..\n");
                ModuleRequest d_req;
                d_req.module_name = module_name;

                byte[] request = new byte[1 + Marshal.SizeOf(typeof(ModuleRequest))];
                request[0] = 0x07;

                IntPtr ptr0 = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(ModuleRequest)));
                Marshal.StructureToPtr(d_req, ptr0, true);
                Marshal.Copy(ptr0, request, 1, Marshal.SizeOf(typeof(ModuleRequest)));
                Marshal.FreeHGlobal(ptr0);

                send_all(request);

                Console.Write("En attente de résultat\n");
                byte[] data = recv_all();

                IntPtr ptr = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(ReturnResponce)));
                Marshal.Copy(data, 0, ptr, Marshal.SizeOf(typeof(ReturnResponce)));
                ReturnResponce r = (ReturnResponce)Marshal.PtrToStructure(ptr, typeof(ReturnResponce));
                ret = r.ret;
            }

            return ret;
        }

        public Module GetModuleInfo(int module_id)
        {
            Module m = new Module();

            lock (ps4_lock)
            {
                ModuleIDRequest i_req;
                i_req.id = module_id;

                byte[] request = new byte[1 + Marshal.SizeOf(typeof(ModuleIDRequest))];
                request[0] = 0x14;

                IntPtr ptr0 = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(ModuleIDRequest)));
                Marshal.StructureToPtr(i_req, ptr0, true);
                Marshal.Copy(ptr0, request, 1, Marshal.SizeOf(typeof(ModuleIDRequest)));
                Marshal.FreeHGlobal(ptr0);

                send_all(request);

                Console.Write("En attente de résultat\n");
                byte[] data = recv_all();

                IntPtr ptr = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(Module)));
                Marshal.Copy(data, 0, ptr, Marshal.SizeOf(typeof(Module)));
                Module r = (Module)Marshal.PtrToStructure(ptr, typeof(Module));
                m = r;
            }

            return m;
        }

        public ulong ResolveFunction(int module_id, string function_name)
        {
            ulong r = 0;

            lock (ps4_lock)
            {
                ResolveRequest r_req;
                r_req.module_id = module_id;
                r_req.func_name = function_name;

                byte[] request = new byte[1 + Marshal.SizeOf(typeof(ResolveRequest)) + 6];
                request[0] = 0x15;

                IntPtr ptr0 = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(ResolveRequest)));
                Marshal.StructureToPtr(r_req, ptr0, true);
                Marshal.Copy(ptr0, request, 1, Marshal.SizeOf(typeof(ResolveRequest)));
                Marshal.FreeHGlobal(ptr0);

                send_all(request);

                Console.Write("En attente de résultat\n");

                byte[] data = recv_all();
                IntPtr ptr = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(UlongResponse)));
                Marshal.Copy(data, 0, ptr, Marshal.SizeOf(typeof(UlongResponse)));
                UlongResponse i = (UlongResponse)Marshal.PtrToStructure(ptr, typeof(UlongResponse));
                r = i.ret;
            }

            return r;
        }

        public int SendPayload(String ip, int port, String path)
        {
            TcpClient payload = new TcpClient();
            try
            {
                payload.Connect(ip, port);
            }
            catch
            {
                return 1;
            }

            byte[] pl;
            try
            {
                pl = File.ReadAllBytes(path);
            }
            catch
            {
                payload.Close();
                return 2;
            }

            try
            {
                NetworkStream payload_steam = payload.GetStream();
                payload_steam.Write(pl, 0, pl.Length);
                payload_steam.Flush();
                payload.Close();
            }
            catch
            {
                payload.Close();
                return 3;
            }

            return 0;
        }

        public void GenericCommand(byte c)
        {
            send_all(new byte[] { c });
        }

        public byte[] GenericCommandWithDump(byte c)
        {
            byte[] data = new byte[] { };

            lock (ps4_lock)
            {
                send_all(new byte[] { c });

                Console.Write("En attente d'une réponse\n");
                data = recv_all();
            }

            return data;
        }

        public int GenericCommandWithRet(byte c)
        {
            ps4.Send(new byte[] { c });

            Console.Write("En attente de résultat\n");
            byte[] data = recv_all();

            IntPtr ptr = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(ReturnResponce)));
            Marshal.Copy(data, 0, ptr, Marshal.SizeOf(typeof(ReturnResponce)));
            ReturnResponce r = (ReturnResponce)Marshal.PtrToStructure(ptr, typeof(ReturnResponce));
            return r.ret;
        }

        public void Buzzer()
        {
            this.GenericCommand(0x17);
        }

        public void SetLED(LED_COLOR color)
        {
            switch (color)
            {
                case LED_COLOR.RED:
                    this.GenericCommand(0x18);
                    break;
                case LED_COLOR.WHITE:
                    this.GenericCommand(0x19);
                    break;
                case LED_COLOR.ORANGE:
                    this.GenericCommand(0x20);
                    break;
                case LED_COLOR.BLUE:
                    this.GenericCommand(0x21);
                    break;
            }
        }
    }
}