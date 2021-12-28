using System;
using System.Collections.Generic;
using System.Windows.Forms;
using Microsoft.Win32;

namespace ManagerF
{
    public partial class Form1 : Form
    {
        string [] extensions =
        {
            "7z",
            "azw",
            "azw3",
            "cb7",
            "cbr",
            "cbt",
            "cbz",
            "epub",
            "epub3",
            "fb2",
            "mobi",
            "rar",
            "rar5",
            "tar",
            "zip",
        };

        public Form1()
        {
            InitializeComponent();
            treeView1.TriStateStyleProperty = RikTheVeggie.TriStateTreeView.TriStateStyles.Installer;
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            treeView1.ExpandAll();

            // update state of nodes from registry
            scanExts();
        }

        private void treeView1_AfterCheck(object sender, TreeViewEventArgs e)
        {

        }

        private void btnApply_Click(object sender, EventArgs e)
        {
            // scan all nodes
            // apply changes to registry
            scanExts2();
        }

        private void btnClose_Click(object sender, EventArgs e)
        {
            Close();
        }

        string DT_GUID = "{70EA7DE2-F3A2-4470-9F39-91C867312B56}";

        private bool isInReg(string ext)
        {
            string start = @"Software\Classes\.";
            string end = @"\ShellEx\{e357fccd-a995-4576-b01f-234630154e96}";
            string full = start + ext + end;
            using (RegistryKey res = Registry.CurrentUser.OpenSubKey(full))
            {
                if (res == null)
                    return false;
                string val = res.GetValue(null).ToString();
                return val == DT_GUID;
            }
        }

        private void setInReg(string ext, bool on)
        {
            string start = @"Software\Classes\.";
            string end = @"\ShellEx\{e357fccd-a995-4576-b01f-234630154e96}";
            string full = start + ext + end;

            using (RegistryKey k1 = Registry.CurrentUser.CreateSubKey(full))
            {
                k1.SetValue(null, on ? DT_GUID : "");
            }
        }

        private void scanExts()
        {
            for (int i=0; i < extensions.Length; i++)
            {
                bool isReg = isInReg(extensions[i]);
                updateTreeNode(extensions[i], isReg);
            }
        }

        private void scanExts2()
        {
            for (int i = 0; i < extensions.Length; i++)
            {
                bool isOn = getTreeNodeState(extensions[i]);
                setInReg(extensions[i], isOn);
            }
        }

        private bool getTreeNodeState(string ext)
        {
            foreach (TreeNode anode in treeView1.GetAllNodes())
            {
                if (anode.Text.ToLower() == ext)
                {
                    return anode.Checked;
                }
            }
            return false;
        }

        private void updateTreeNode(string ext, bool state)
        {
            foreach (TreeNode anode in treeView1.GetAllNodes())
            {
                if (anode.Text.ToLower() == ext)
                {
                    anode.Checked = state ? true : false;
                    break;
                }
            }
        }
    }
}
