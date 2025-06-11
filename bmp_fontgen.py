import tkinter as tk
from tkinter import filedialog, messagebox

CELL_PIXEL_SIZE = 20 

class GlyphDesigner:
    def __init__(self, root):
        self.root = root
        self.root.title("Glyph Designer")

        self.N = 8
        self.grid_data = []
        self.rects = {}
        self.output_path = tk.StringVar()
        self.glyph_name = tk.StringVar(value="charX")

        ctrl_frame = tk.Frame(root)
        ctrl_frame.pack(padx=10, pady=5, anchor="w")

        tk.Label(ctrl_frame, text="Grid size (N):").grid(row=0, column=0, sticky="w")
        self.size_entry = tk.Entry(ctrl_frame, width=4)
        self.size_entry.insert(0, str(self.N))
        self.size_entry.grid(row=0, column=1, sticky="w")
        tk.Button(ctrl_frame, text="Set Grid", command=self.set_grid).grid(row=0, column=2, padx=5)

        tk.Label(ctrl_frame, text="Glyph name:").grid(row=1, column=0, sticky="w")
        tk.Entry(ctrl_frame, textvariable=self.glyph_name, width=12).grid(row=1, column=1, columnspan=2, sticky="w")

        tk.Label(ctrl_frame, text="Output file:").grid(row=2, column=0, sticky="w")
        tk.Entry(ctrl_frame, textvariable=self.output_path, width=30).grid(row=2, column=1, sticky="w")
        tk.Button(ctrl_frame, text="Browse…", command=self.browse_file).grid(row=2, column=2, padx=5)

        self.canvas_frame = tk.Frame(root)
        self.canvas_frame.pack(padx=10, pady=5)
        self.canvas = tk.Canvas(self.canvas_frame,
                                width=self.N*CELL_PIXEL_SIZE,
                                height=self.N*CELL_PIXEL_SIZE,
                                bg="white")
        self.canvas.pack()
        self.canvas.bind("<Button-1>", self.on_canvas_click)

        tk.Button(root, text="Generate", command=self.generate_array).pack(pady=5)
        self.set_grid()

    def set_grid(self):
        try:
            N = int(self.size_entry.get())
            if N <= 0:
                raise ValueError
        except ValueError:
            messagebox.showerror("Invalid size", "Grid size must be a positive integer.")
            return

        self.N = N
        self.grid_data = [[False]*self.N for _ in range(self.N)]
        self.canvas.config(width=self.N*CELL_PIXEL_SIZE,
                           height=self.N*CELL_PIXEL_SIZE)
        self.redraw_grid()

    def redraw_grid(self):
        self.canvas.delete("all")
        self.rects.clear()
        for r in range(self.N):
            for c in range(self.N):
                x0 = c*CELL_PIXEL_SIZE
                y0 = r*CELL_PIXEL_SIZE
                rect = self.canvas.create_rectangle(
                    x0, y0,
                    x0+CELL_PIXEL_SIZE, y0+CELL_PIXEL_SIZE,
                    fill="white", outline="gray"
                )
                self.rects[(r,c)] = rect

    def on_canvas_click(self, event):
        c = event.x // CELL_PIXEL_SIZE
        r = event.y // CELL_PIXEL_SIZE
        if 0 <= r < self.N and 0 <= c < self.N:
            self.grid_data[r][c] = not self.grid_data[r][c]
            color = "black" if self.grid_data[r][c] else "white"
            self.canvas.itemconfigure(self.rects[(r,c)], fill=color)

    def browse_file(self):
        path = filedialog.askopenfilename(
            title="Select output file",
            filetypes=[("C files", "*.c;*.h"), ("All files","*.*")]
        )
        if path:
            self.output_path.set(path)

    def generate_array(self):
        name = self.glyph_name.get().strip()
        path = self.output_path.get().strip()
        if not name:
            messagebox.showerror("No name", "Please enter a glyph name.")
            return
        if not path:
            messagebox.showerror("No file", "Please select an output file.")
            return

        byte_count = self.N * self.N * 3
        if byte_count % 32 != 0:
            messagebox.showerror(
                "Alignment error",
                f"Your grid ({self.N}×{self.N}) generates {byte_count} bytes, "
                "which is not a multiple of 32. Please pick a different size."
            )
            return

        flat = []
        for row in self.grid_data:
            for cell in row:
                if cell:  
                    flat.extend((0, 0, 0))
                else:    
                    flat.extend((255, 255, 255))

        data_str = ", ".join(str(v) for v in flat)

        c_decl = (
            f"_Alignas(32) unsigned char bmp_{name}_data[] = {{ {data_str} }};\n"
        )

        try:
            with open(path, "a") as f:
                f.write(c_decl)
        except Exception as e:
            messagebox.showerror("Write error", f"Could not write to file:\n{e}")
            return

if __name__ == "__main__":
    root = tk.Tk()
    app = GlyphDesigner(root)
    root.mainloop()
