using Be.Windows.Forms;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace PS4TOOL
{
    public partial class Debug : Form
    {
        PS4RTE ps4;
        int process_id;

        public Debug(int pid, PS4RTE parent)
        {
            ps4 = parent;
            process_id = pid;
            InitializeComponent();
        }

        private void refresh()
        {
            ulong offset = UInt64.Parse(textBox1.Text, System.Globalization.NumberStyles.HexNumber);
            byte[] data = ps4.GetMemory(process_id, offset, (int)numericUpDown1.Value);

            DynamicByteProvider dbp = new DynamicByteProvider(data);
            hexBox1.ByteProvider = dbp;
            hexBox1.LineInfoOffset = (long)offset;
        }

        private void Debug_Load(object sender, EventArgs e)
        {
            refresh();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            refresh();
        }

        private byte[] ByteProviderToArray(IByteProvider b)
        {
            byte[] data = new byte[b.Length];
            for (int i = 0; i<b.Length; i++)
            {
                data[i] = b.ReadByte(i);
            }
            return data;
        }

        private void hexBox1_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (e.KeyChar == (char)13)
            {
                ulong offset = UInt64.Parse(textBox1.Text, System.Globalization.NumberStyles.HexNumber);
                ps4.SetMemory(process_id, offset, ByteProviderToArray(hexBox1.ByteProvider));
                MessageBox.Show("Memory set");
            }
        }

        public static byte[] StringToByteArray(String hex)
        {
            int NumberChars = hex.Length;
            byte[] bytes = new byte[NumberChars / 2];
            for (int i = 0; i < NumberChars; i += 2)
                bytes[i / 2] = Convert.ToByte(hex.Substring(i, 2), 16);
            return bytes;
        }

        static int search(byte[] haystack, byte[] needle, int go_to)
        {
            for (int i = go_to; i <= haystack.Length - needle.Length; i++)
            {
                if (match(haystack, needle, i))
                {
                    return i;
                }
            }
            return -1;
        }

        static bool match(byte[] haystack, byte[] needle, int start)
        {
            if (needle.Length + start > haystack.Length)
            {
                return false;
            }
            else
            {
                for (int i = 0; i < needle.Length; i++)
                {
                    if (needle[i] != haystack[i + start])
                    {
                        return false;
                    }
                }
                return true;
            }
        }

        private void button2_Click(object sender, EventArgs e)
        {
            dataGridView1.Rows.Clear();

            refresh();
            Console.Write("Refresh complete\n");


            ulong offset = UInt64.Parse(textBox1.Text, System.Globalization.NumberStyles.HexNumber);
            byte[] all_data = ByteProviderToArray(hexBox1.ByteProvider);
            byte[] searched_data = StringToByteArray(textBox2.Text.Replace("-", ""));

            Console.Write("Launching search ...\n");

            int r = 0;
            int go = 0;
            while (true)
            {
                if (go == 1)
                {
                    r++;
                }

                r = search(all_data, searched_data, r);
                if (r == -1)
                    break;
                Console.Write("r: " + r.ToString() + "\n");
                ulong c_o = offset + Convert.ToUInt64(r);
                Console.Write("Found: " + c_o.ToString("X") + "\n");
                dataGridView1.Rows.Add("0x" + c_o.ToString("X"));

                go = 1;
            }

            MessageBox.Show("Recherche terminer");
        }

        private void button3_Click(object sender, EventArgs e)
        {
            refresh();
            ulong b_offset = UInt64.Parse(textBox1.Text, System.Globalization.NumberStyles.HexNumber);
            byte[] all_data = ByteProviderToArray(hexBox1.ByteProvider);
            byte[] searched_data = StringToByteArray(textBox2.Text.Replace("-", ""));

            for (int i = 0; i<dataGridView1.RowCount-1; i++)
            {
                Console.Write(dataGridView1.Rows[i]);
                ulong offset = UInt64.Parse(dataGridView1.Rows[i].Cells["Offset"].Value.ToString().Substring(2), System.Globalization.NumberStyles.HexNumber);
                Console.Write("Offset: " + offset.ToString("X") + "\n");
                int data_pos = Convert.ToInt32(offset - b_offset);
                if (!match(all_data, searched_data, data_pos))
                {
                    dataGridView1.Rows.RemoveAt(i);
                }
            }

            MessageBox.Show("Recherche suivante terminer");
        }

        private void button4_Click(object sender, EventArgs e)
        {
            dataGridView1.Rows.Clear();
        }

        private void dataGridView1_CellClick(object sender, DataGridViewCellEventArgs e)
        {
            textBox1.Text = dataGridView1.Rows[e.RowIndex].Cells["Offset"].Value.ToString().Substring(2);
        }

        private void button5_Click(object sender, EventArgs e)
        {
            byte[] data = StringToByteArray(textBox2.Text.Replace("-", ""));

            for (int i = 0; i < dataGridView1.RowCount+1; i++)
            {
                Console.Write("i: " + i.ToString() + "\n");
                ulong offset = UInt64.Parse(dataGridView1.Rows[0].Cells["Offset"].Value.ToString().Substring(2), System.Globalization.NumberStyles.HexNumber);
                ps4.SetMemory(process_id, offset, data);
                DialogResult dR = MessageBox.Show(dataGridView1.Rows[0].Cells["Offset"].Value.ToString() + " ?", "Step Change", MessageBoxButtons.YesNo);
                if (dR == DialogResult.Yes)
                {
                    for (int a = 1; a < dataGridView1.RowCount; a++)
                    {   try
                        {
                            dataGridView1.Rows.RemoveAt(a);
                        } catch (Exception puute)
                        {

                        }
                    }
                    break;
                }
                dataGridView1.Rows.RemoveAt(0);
            }

            MessageBox.Show("Step change ended");
            refresh();
        }

        private void textBox1_TextChanged(object sender, EventArgs e)
        {

        }

        private void textBox5_TextChanged(object sender, EventArgs e)
        {

        }

        private void label5_Click(object sender, EventArgs e)
        {

        }

        private void textBox3_TextChanged(object sender, EventArgs e)
        {

        }
    }
}
