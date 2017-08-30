using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace PS4TOOL
{
    public partial class progressForm : Form
    {
        private bool finish;
        private PS4RTE ps4;
        private int pid;
        private ulong offset;
        private int size;

        public byte[] data;

        public progressForm(PS4RTE p, int proc_id, ulong off, int s)
        {
            InitializeComponent();
            ps4 = p;
            pid = proc_id;
            offset = off;
            size = s;
        }

        private void download(object sender, DoWorkEventArgs e)
        {
            this.data = ps4.GetMemory(pid, offset, size);
            finish = true;
        }

        private void update(object sender, DoWorkEventArgs e)
        {
            while (!finish)
            {
                progressBar1.Invoke(new Action(() =>
                {
                    progressBar1.Value = ps4.recv_prec;
                }));

                label2.Invoke(new Action(() =>
                {
                    label2.Text = ps4.recv_prec.ToString() + " %";
                }));

                Thread.Sleep(1000);
            }

            this.Invoke((MethodInvoker)delegate
            {
                this.Close();
            });
        }

        private void progressForm_Load(object sender, EventArgs e)
        {
            finish = false;

            BackgroundWorker dw = new BackgroundWorker();
            dw.DoWork += download;
            dw.RunWorkerAsync();

            BackgroundWorker uw = new BackgroundWorker();
            uw.DoWork += update;
            uw.RunWorkerAsync();
        }

        public void setTitle(string title)
        {
            this.Text = title;
        }

        public void setText(string text)
        {
            label1.Text = text;
        }

        public void setProgress(int p)
        {
            progressBar1.Value = p;
        }
    }
}
