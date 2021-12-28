
namespace ManagerF
{
    partial class Form1
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.Windows.Forms.TreeNode treeNode1 = new System.Windows.Forms.TreeNode("7Z");
            System.Windows.Forms.TreeNode treeNode2 = new System.Windows.Forms.TreeNode("RAR");
            System.Windows.Forms.TreeNode treeNode3 = new System.Windows.Forms.TreeNode("RAR5");
            System.Windows.Forms.TreeNode treeNode4 = new System.Windows.Forms.TreeNode("TAR");
            System.Windows.Forms.TreeNode treeNode5 = new System.Windows.Forms.TreeNode("ZIP");
            System.Windows.Forms.TreeNode treeNode6 = new System.Windows.Forms.TreeNode("Archive", new System.Windows.Forms.TreeNode[] {
            treeNode1,
            treeNode2,
            treeNode3,
            treeNode4,
            treeNode5});
            System.Windows.Forms.TreeNode treeNode7 = new System.Windows.Forms.TreeNode("CB7");
            System.Windows.Forms.TreeNode treeNode8 = new System.Windows.Forms.TreeNode("CBR");
            System.Windows.Forms.TreeNode treeNode9 = new System.Windows.Forms.TreeNode("CBT");
            System.Windows.Forms.TreeNode treeNode10 = new System.Windows.Forms.TreeNode("CBZ");
            System.Windows.Forms.TreeNode treeNode11 = new System.Windows.Forms.TreeNode("Comic Book", new System.Windows.Forms.TreeNode[] {
            treeNode7,
            treeNode8,
            treeNode9,
            treeNode10});
            System.Windows.Forms.TreeNode treeNode12 = new System.Windows.Forms.TreeNode("EPUB");
            System.Windows.Forms.TreeNode treeNode13 = new System.Windows.Forms.TreeNode("EPUB3");
            System.Windows.Forms.TreeNode treeNode14 = new System.Windows.Forms.TreeNode("Epub", new System.Windows.Forms.TreeNode[] {
            treeNode12,
            treeNode13});
            System.Windows.Forms.TreeNode treeNode15 = new System.Windows.Forms.TreeNode("FB2");
            System.Windows.Forms.TreeNode treeNode16 = new System.Windows.Forms.TreeNode("Fiction Book", new System.Windows.Forms.TreeNode[] {
            treeNode15});
            System.Windows.Forms.TreeNode treeNode17 = new System.Windows.Forms.TreeNode("AZW");
            System.Windows.Forms.TreeNode treeNode18 = new System.Windows.Forms.TreeNode("AZW3");
            System.Windows.Forms.TreeNode treeNode19 = new System.Windows.Forms.TreeNode("MOBI");
            System.Windows.Forms.TreeNode treeNode20 = new System.Windows.Forms.TreeNode("Kindle", new System.Windows.Forms.TreeNode[] {
            treeNode17,
            treeNode18,
            treeNode19});
            System.Windows.Forms.TreeNode treeNode21 = new System.Windows.Forms.TreeNode("All Formats", new System.Windows.Forms.TreeNode[] {
            treeNode6,
            treeNode11,
            treeNode14,
            treeNode16,
            treeNode20});
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Form1));
            this.treeView1 = new RikTheVeggie.TriStateTreeView();
            this.checkBox1 = new System.Windows.Forms.CheckBox();
            this.button1 = new System.Windows.Forms.Button();
            this.button2 = new System.Windows.Forms.Button();
            this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
            this.textBox1 = new System.Windows.Forms.TextBox();
            this.SuspendLayout();
            // 
            // treeView1
            // 
            this.treeView1.Location = new System.Drawing.Point(12, 60);
            this.treeView1.Name = "treeView1";
            treeNode1.Name = "Node2";
            treeNode1.StateImageIndex = 0;
            treeNode1.Text = "7Z";
            treeNode2.Name = "Node3";
            treeNode2.StateImageIndex = 0;
            treeNode2.Text = "RAR";
            treeNode3.Name = "Node4";
            treeNode3.StateImageIndex = 0;
            treeNode3.Text = "RAR5";
            treeNode4.Name = "Node5";
            treeNode4.StateImageIndex = 0;
            treeNode4.Text = "TAR";
            treeNode5.Name = "Node17";
            treeNode5.StateImageIndex = 0;
            treeNode5.Text = "ZIP";
            treeNode6.Name = "Node1";
            treeNode6.StateImageIndex = 0;
            treeNode6.Text = "Archive";
            treeNode6.ToolTipText = "Shows image within archive files";
            treeNode7.Name = "Node13";
            treeNode7.StateImageIndex = 0;
            treeNode7.Text = "CB7";
            treeNode8.Name = "Node14";
            treeNode8.StateImageIndex = 0;
            treeNode8.Text = "CBR";
            treeNode9.Name = "Node15";
            treeNode9.StateImageIndex = 0;
            treeNode9.Text = "CBT";
            treeNode10.Name = "Node16";
            treeNode10.StateImageIndex = 0;
            treeNode10.Text = "CBZ";
            treeNode11.Name = "Node6";
            treeNode11.StateImageIndex = 0;
            treeNode11.Text = "Comic Book";
            treeNode11.ToolTipText = "Shows image within comic book archives";
            treeNode12.Name = "Node0";
            treeNode12.Text = "EPUB";
            treeNode13.Name = "Node1";
            treeNode13.Text = "EPUB3";
            treeNode14.Name = "Node7";
            treeNode14.StateImageIndex = 0;
            treeNode14.Text = "Epub";
            treeNode14.ToolTipText = "Shows the cover image within epub files";
            treeNode15.Name = "Node2";
            treeNode15.Text = "FB2";
            treeNode16.Name = "Node8";
            treeNode16.StateImageIndex = 0;
            treeNode16.Text = "Fiction Book";
            treeNode16.ToolTipText = "Shows the cover of .fb2 files";
            treeNode17.Name = "Node10";
            treeNode17.StateImageIndex = 0;
            treeNode17.Text = "AZW";
            treeNode18.Name = "Node11";
            treeNode18.StateImageIndex = 0;
            treeNode18.Text = "AZW3";
            treeNode19.Name = "Node12";
            treeNode19.StateImageIndex = 0;
            treeNode19.Text = "MOBI";
            treeNode20.Name = "Node9";
            treeNode20.StateImageIndex = 0;
            treeNode20.Text = "Kindle";
            treeNode20.ToolTipText = "Shows the cover for Kindle format files";
            treeNode21.Name = "Node0";
            treeNode21.StateImageIndex = 0;
            treeNode21.Text = "All Formats";
            this.treeView1.Nodes.AddRange(new System.Windows.Forms.TreeNode[] {
            treeNode21});
            this.treeView1.ShowNodeToolTips = true;
            this.treeView1.Size = new System.Drawing.Size(233, 305);
            this.treeView1.TabIndex = 1;
            this.treeView1.TriStateStyleProperty = RikTheVeggie.TriStateTreeView.TriStateStyles.Standard;
            this.treeView1.AfterCheck += new System.Windows.Forms.TreeViewEventHandler(this.treeView1_AfterCheck);
            // 
            // checkBox1
            // 
            this.checkBox1.AutoSize = true;
            this.checkBox1.Location = new System.Drawing.Point(12, 383);
            this.checkBox1.Name = "checkBox1";
            this.checkBox1.Size = new System.Drawing.Size(148, 17);
            this.checkBox1.TabIndex = 2;
            this.checkBox1.Text = "Sort images alphabetically";
            this.toolTip1.SetToolTip(this.checkBox1, "Uncheck to sort images by archive order.\\nRequired to display custom thumbnail.");
            this.checkBox1.UseVisualStyleBackColor = true;
            // 
            // button1
            // 
            this.button1.Location = new System.Drawing.Point(12, 418);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(75, 23);
            this.button1.TabIndex = 3;
            this.button1.Text = "Apply";
            this.button1.UseVisualStyleBackColor = true;
            this.button1.Click += new System.EventHandler(this.btnApply_Click);
            // 
            // button2
            // 
            this.button2.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.button2.Location = new System.Drawing.Point(94, 418);
            this.button2.Name = "button2";
            this.button2.Size = new System.Drawing.Size(75, 23);
            this.button2.TabIndex = 4;
            this.button2.Text = "Close";
            this.button2.UseVisualStyleBackColor = true;
            this.button2.Click += new System.EventHandler(this.btnClose_Click);
            // 
            // textBox1
            // 
            this.textBox1.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.textBox1.Location = new System.Drawing.Point(12, 12);
            this.textBox1.Multiline = true;
            this.textBox1.Name = "textBox1";
            this.textBox1.ReadOnly = true;
            this.textBox1.Size = new System.Drawing.Size(229, 42);
            this.textBox1.TabIndex = 5;
            this.textBox1.TabStop = false;
            this.textBox1.Text = "Select which file format(s) you wish to \r\nsee thumbnails for:";
            this.toolTip1.SetToolTip(this.textBox1, "May require clearing the thumbnail cache via Disk Cleanup.");
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(259, 450);
            this.Controls.Add(this.textBox1);
            this.Controls.Add(this.button2);
            this.Controls.Add(this.button1);
            this.Controls.Add(this.checkBox1);
            this.Controls.Add(this.treeView1);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "Form1";
            this.Text = "DarkThumbs Manager";
            this.Load += new System.EventHandler(this.Form1_Load);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion
        private RikTheVeggie.TriStateTreeView treeView1;
        private System.Windows.Forms.CheckBox checkBox1;
        private System.Windows.Forms.Button button1;
        private System.Windows.Forms.Button button2;
        private System.Windows.Forms.ToolTip toolTip1;
        private System.Windows.Forms.TextBox textBox1;
    }
}

