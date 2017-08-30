using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using Be;
using System.Threading;

namespace PS4TOOL
{
    public partial class Form1 : Form
    {
        PS4RTE ps4 = new PS4RTE();
        int c_p = 0;

        public Form1()
        {
            InitializeComponent();
        }

        private void update_processlist() {
            comboBox1.Items.Clear();
            PS4TOOL.PS4RTE.Process[] proc = ps4.GetProcesses();
            for (int i = 0; i < proc.Length; i++)
            {
                ComboboxItem item = new ComboboxItem();
                item.Text = proc[i].process_name + " (" + proc[i].process_id.ToString() + ")";
                item.Value = proc[i].process_id;

                comboBox1.Items.Add(item);
            }
        }

        private void update_pages()
        {
            dataGridView1.Rows.Clear();

            ComboboxItem item = (ComboboxItem)comboBox1.SelectedItem;
            PS4TOOL.PS4RTE.Page[] p = ps4.GetPages((int)item.Value);

            for (int i = 0; i < p.Length; i++)
            {
                dataGridView1.Rows.Add(i.ToString(), "0x" + p[i].start.ToString("X"), "0x" + p[i].stop.ToString("X"), "0x" + (p[i].stop - p[i].start).ToString("X"), ps4.GetPerm(p[i].protection));
            }
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if (ps4.Connect(textBox1.Text))
            {
                MessageBox.Show("OPENRTE - Playstation 4 Connected, have fun !!!");
                ps4.SendNotify("Playstation 4 Tool - Connected !");
            }
            else
            {
                MessageBox.Show("OPENRTE - Unable to connect to the Playstation 4 !");
            }
        }

        private void button2_Click(object sender, EventArgs e)
        {
            update_processlist();
        }

        private void button3_Click(object sender, EventArgs e)
        {
            int result = ps4.SendPayload(textBox1.Text, 5053, "openrte.elf");

            switch (result)
            {
                case 0:
                    MessageBox.Show("Payload Envoyer !");
                    break;
                case 1:
                    MessageBox.Show("Unable to connect to the Playstation 4");
                    break;
                case 2:
                    MessageBox.Show("Unable to open the payload file !");
                    break;
                case 3:
                    MessageBox.Show("Unable to write the payload !");
                    break;
            }
        }

        private void comboBox1_SelectedIndexChanged(object sender, EventArgs e)
        {
            update_pages();
        }

        private void button1_Click_1(object sender, EventArgs e)
        {
            update_pages();
        }

        private void dumpSPageButton_Click(object sender, EventArgs e)
        {
            ComboboxItem item = (ComboboxItem)comboBox1.SelectedItem;
            int process_id = (int)item.Value;
            string process_name = item.Text;
            ulong origin = UInt64.Parse(dataGridView1.Rows[0].Cells["Start"].Value.ToString().Substring(2), System.Globalization.NumberStyles.HexNumber);
            ulong start = UInt64.Parse(dataGridView1.SelectedRows[0].Cells["Start"].Value.ToString().Substring(2), System.Globalization.NumberStyles.HexNumber);
            ulong stop = UInt64.Parse(dataGridView1.SelectedRows[0].Cells["Stop"].Value.ToString().Substring(2), System.Globalization.NumberStyles.HexNumber);
            int size = unchecked((int)stop - (int)start);
            ulong offset = start - origin;

            SaveFileDialog s_dialog = new SaveFileDialog();
            s_dialog.FileName = process_name + "_0x" + start.ToString("X") + "_0x" + stop.ToString("X") + ".bin";
            if (s_dialog.ShowDialog() == DialogResult.OK)
            {
                string save_path = s_dialog.FileName;
                progressForm pf = new progressForm(ps4, process_id, offset, size);
                pf.ShowDialog();
                File.WriteAllBytes(save_path, pf.data);

                MessageBox.Show("Page dumped !");
            }
        }

        private void get_Click(object sender, EventArgs e)
        {
            ComboboxItem item = (ComboboxItem)comboBox1.SelectedItem;
            int process_id = (int)item.Value;

            ulong offset = UInt64.Parse(textBox2.Text, System.Globalization.NumberStyles.HexNumber);


            byte[] data = ps4.GetMemory(process_id, offset, Convert.ToInt32(numericUpDown1.Value));
            textBox3.Text = BitConverter.ToString(data);
        }

        public static byte[] StringToByteArray(String hex)
        {
            int NumberChars = hex.Length;
            byte[] bytes = new byte[NumberChars / 2];
            for (int i = 0; i < NumberChars; i += 2)
                bytes[i / 2] = Convert.ToByte(hex.Substring(i, 2), 16);
            return bytes;
        }

        private void button2_Click_1(object sender, EventArgs e)
        {
            ComboboxItem item = (ComboboxItem)comboBox1.SelectedItem;
            int process_id = (int)item.Value;

            ulong offset = UInt64.Parse(textBox4.Text, System.Globalization.NumberStyles.HexNumber);

            byte[] data = StringToByteArray(textBox5.Text.Replace("-", ""));
            ps4.SetMemory(process_id, offset, data);
        }

        private void dumpAllPageButton_Click(object sender, EventArgs e)
        {
            ComboboxItem item = (ComboboxItem)comboBox1.SelectedItem;
            int process_id = (int)item.Value;
            string process_name = item.Text;

            FolderBrowserDialog dialog = new FolderBrowserDialog();
            if (dialog.ShowDialog() == DialogResult.OK)
            {
                ulong origin = UInt64.Parse(dataGridView1.Rows[0].Cells["Start"].Value.ToString().Substring(2), System.Globalization.NumberStyles.HexNumber);

                for (int i = 0; i < dataGridView1.RowCount; i++)
                {
                    ulong start = UInt64.Parse(dataGridView1.Rows[i].Cells["Start"].Value.ToString().Substring(2), System.Globalization.NumberStyles.HexNumber);
                    ulong stop = UInt64.Parse(dataGridView1.Rows[i].Cells["Stop"].Value.ToString().Substring(2), System.Globalization.NumberStyles.HexNumber);
                    int size = unchecked((int)stop - (int)start);
                    ulong offset = start - origin;

                    

                    string save_path = dialog.SelectedPath + "\\" + process_name + "_0x" + start.ToString("X") + "_0x" + stop.ToString("X") + ".bin";;
                    Console.Write("Dumping: " + save_path);
                    byte[] data = ps4.GetMemory(process_id, offset, size);
                    File.WriteAllBytes(save_path, data);
                }

                MessageBox.Show("Dump ended !");
            }
        }

        private void button3_Click_1(object sender, EventArgs e)
        {
            if (comboBox1.Text == "")
            {
                MessageBox.Show("Select a process first !");
            }
            else
            {
                ComboboxItem item = (ComboboxItem)comboBox1.SelectedItem;
                int process_id = (int)item.Value;

                Debug form_debug = new Debug(process_id, ps4);
                form_debug.Show();
            }
        }

        private void button9_Click(object sender, EventArgs e)
        {
            ps4.Exit();
        }

        private void button10_Click(object sender, EventArgs e)
        {
            byte[] data = ps4.DumpModule(textBox7.Text);
            if (data.Length != 0)
            {
                SaveFileDialog s_dialog = new SaveFileDialog();
                s_dialog.FileName = textBox7.Text + ".bin";
                if (s_dialog.ShowDialog() == DialogResult.OK)
                {
                    string save_path = s_dialog.FileName;
                    File.WriteAllBytes(save_path, data);

                    MessageBox.Show("Module dumperd !");
                } else
                {
                    MessageBox.Show("Your have abord the save");
                }
            }
            else
            {
                MessageBox.Show("Unable to dump the module !");
            }
        }

        private void button11_Click(object sender, EventArgs e)
        {
            int r = ps4.LoadModule(textBox8.Text);
            MessageBox.Show(r.ToString());
        }

        private void button4_Click(object sender, EventArgs e)
        {
            ps4.SendNotify(textBox6.Text);
        }

        private void button5_Click(object sender, EventArgs e)
        {
            ps4.GenericCommand(0x16);
            MessageBox.Show("Test sended");
        }

        private void button6_Click(object sender, EventArgs e)
        {
            byte[] data = ps4.GenericCommandWithDump(0x09);
            if (data.Length != 0)
            {
                SaveFileDialog s_dialog = new SaveFileDialog();
                s_dialog.FileName = "idps.bin";
                if (s_dialog.ShowDialog() == DialogResult.OK)
                {
                    string save_path = s_dialog.FileName;
                    File.WriteAllBytes(save_path, data);

                    MessageBox.Show("Dumped !");
                }
                else
                {
                    MessageBox.Show("You have abord the save");
                }
            }
            else
            {
                MessageBox.Show("Unable to dump !");
            }
        }

        private void button7_Click(object sender, EventArgs e)
        {
            PS4RTE.Module m = ps4.GetModuleInfo(Convert.ToInt32(numericUpDown2.Value));
            MessageBox.Show("Module name: " + m.name + Environment.NewLine + "Module base: 0x" + m.base_addr.ToString("X") + Environment.NewLine + "Module Size: " + m.size.ToString());
        }

        private void button8_Click(object sender, EventArgs e)
        {
            ulong addr = ps4.ResolveFunction(Convert.ToInt32(numericUpDown3.Value), textBox9.Text);
            MessageBox.Show("0x" + addr.ToString("X"));
        }

        private void button12_Click(object sender, EventArgs e)
        {

        }

        private void button13_Click(object sender, EventArgs e)
        {
            ps4.SetLED(PS4RTE.LED_COLOR.BLUE);
        }

        private void button14_Click(object sender, EventArgs e)
        {

        }

        private void button14_Click_1(object sender, EventArgs e)
        {

        }

        private void button14_Click_2(object sender, EventArgs e)
        {
            ps4.Buzzer();
        }

        private void button12_Click_2(object sender, EventArgs e)
        {
            ps4.SetLED(PS4RTE.LED_COLOR.ORANGE);
        }

        private void button15_Click(object sender, EventArgs e)
        {
            ps4.SetLED(PS4RTE.LED_COLOR.RED);
        }

        private void button16_Click(object sender, EventArgs e)
        {
            ps4.SetLED(PS4RTE.LED_COLOR.WHITE);
        }

        private void Form1_Load(object sender, EventArgs e)
        {

        }
    }

    public class ComboboxItem
    {
        public string Text { get; set; }
        public object Value { get; set; }

        public override string ToString()
        {
            return Text;
        }
    }
}
