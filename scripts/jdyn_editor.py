#!/usr/bin/env python3
"""
jdyn_editor.py -- GUI editor for the JDYN and THRS chunks of Strike
Commander .IFF jet object files.

Values are edited and written back IN PLACE: the chunk layout and
size never change (every field has a fixed width), only the raw
bytes at each field's known offset are overwritten. Everything else
in the file (geometry, textures, other chunks) is left untouched.

Usage:
    python3 jdyn_editor.py [file.IFF]

Requires only the standard library (tkinter).
"""
import struct
import sys
import tkinter as tk
from tkinter import ttk, filedialog, messagebox


# ---------------------------------------------------------------------
# Field layout (offset, size in bytes, type, name)
# type is one of: "fixed24.8", "u8", "i8", "u16", "i16", "u32", "i32"
# ---------------------------------------------------------------------

JDYN_FIELDS = [
    (0,  4, "fixed24.8", "FUEL"),
    (4,  4, "fixed24.8", "field_new_02"),
    (8,  4, "fixed24.8", "boost_modifier_a"),
    (12, 4, "fixed24.8", "boost_modifier_b"),
    (16, 4, "fixed24.8", "drag_modifier_a"),
    (20, 4, "fixed24.8", "drag_modifier_b"),
    (24, 4, "fixed24.8", "inertia_like"),
    (28, 4, "fixed24.8", "pitch_rate_gain"),
    (32, 1, "u8",        "rate_threshold"),
    (33, 1, "u8",        "ctrl_modifier_a"),
    (34, 1, "u8",        "ctrl_modifier_b"),
    (35, 4, "fixed24.8", "envelope_vs_limit"),
    (39, 4, "fixed24.8", "envelope_speed_limit"),
    (43, 1, "u8",        "envelope_bank_limit"),
    (44, 1, "u8",        "envelope_pitch_limit"),
    (45, 1, "u8",        "envelope_pitch_margin"),
    (46, 4, "fixed24.8", "min_airspeed"),
    (50, 4, "fixed24.8", "drag_coefficient"),
    (54, 4, "fixed24.8", "airframe_response_scale"),
    (58, 1, "i8",        "aileron"),
    (59, 1, "u8",        "gouverne"),
    (60, 1, "i8",        "MAX_G"),
    (61, 2, "i16",       "field_22"),
    (63, 2, "i16",       "field_23"),
    (65, 2, "i16",       "field_24"),
    (67, 4, "i32",       "field_25"),
    (71, 1, "i8",        "field_26"),
    (72, 1, "i8",        "field_27"),
]
JDYN_SIZE = 73

THRS_FIELDS = [
    (0, 4, "fixed24.8", "thrust_base"),
    (4, 1, "i8",        "afterburner_flag"),
    (5, 1, "i8",        "engine_param_1"),
    (6, 1, "i8",        "engine_param_2"),
]
THRS_SIZE = 7


# ---------------------------------------------------------------------
# IFF chunk walking
# ---------------------------------------------------------------------

class Chunk:
    """A located chunk: its tag, the file offset of its DATA (right
    after the 8-byte tag+size header), and its raw data bytes."""
    def __init__(self, tag, data_offset, data):
        self.tag = tag
        self.data_offset = data_offset
        self.data = data


def walk_iff(data, offset=0, end=None, path=""):
    """Recursively walk an IFF file (big-endian tag+size headers,
    FORM/LIST/CAT containers with a 4-char sub-id). Returns a flat
    list of Chunk objects, including container chunks themselves."""
    if end is None:
        end = len(data)
    chunks = []
    off = offset
    while off + 8 <= end:
        tag = data[off:off + 4].decode("ascii", "replace")
        size = struct.unpack(">I", data[off + 4:off + 8])[0]
        data_off = off + 8
        chunks.append(Chunk(tag, data_off, data[data_off:data_off + size]))
        if tag in ("FORM", "LIST", "CAT "):
            sub_id = data[data_off:data_off + 4].decode("ascii", "replace")
            chunks.extend(walk_iff(data, data_off + 4, min(data_off + size, end),
                                    path + "/" + tag + ":" + sub_id))
        off = data_off + size
        if size % 2 == 1:
            off += 1  # IFF pads chunks to an even boundary
    return chunks


def find_all(data, tag):
    """Return every Chunk in the file matching `tag`, in file order."""
    return [c for c in walk_iff(data) if c.tag == tag]


# ---------------------------------------------------------------------
# Field <-> raw bytes conversion
# ---------------------------------------------------------------------

def read_field(raw, offset, size, ftype):
    chunk = raw[offset:offset + size]
    if ftype == "fixed24.8":
        v = int.from_bytes(chunk, "little", signed=True)
        return v / 256.0
    signed = ftype.startswith("i")
    return int.from_bytes(chunk, "little", signed=signed)


def write_field(value, size, ftype):
    if ftype == "fixed24.8":
        raw_int = round(float(value) * 256)
        return int(raw_int).to_bytes(size, "little", signed=True)
    signed = ftype.startswith("i")
    return int(value).to_bytes(size, "little", signed=signed)


def field_range(size, ftype):
    """Human hint for the valid integer range of a non-fixed field."""
    if ftype == "fixed24.8":
        return None
    bits = size * 8
    if ftype.startswith("i"):
        lo, hi = -(2 ** (bits - 1)), 2 ** (bits - 1) - 1
    else:
        lo, hi = 0, 2 ** bits - 1
    return lo, hi


# ---------------------------------------------------------------------
# GUI
# ---------------------------------------------------------------------

class ScrollableFrame(ttk.Frame):
    """A vertically scrollable container. Put widgets inside `.inner`
    instead of `self` directly."""

    def __init__(self, parent):
        super().__init__(parent)
        canvas = tk.Canvas(self, borderwidth=0, highlightthickness=0)
        scrollbar = ttk.Scrollbar(self, orient="vertical", command=canvas.yview)
        self.inner = ttk.Frame(canvas)

        self.inner.bind(
            "<Configure>",
            lambda e: canvas.configure(scrollregion=canvas.bbox("all")))

        window_id = canvas.create_window((0, 0), window=self.inner, anchor="nw")
        canvas.configure(yscrollcommand=scrollbar.set)

        def _match_inner_width(event):
            canvas.itemconfig(window_id, width=event.width)
        canvas.bind("<Configure>", _match_inner_width)

        def _on_mousewheel(event):
            if event.num == 4:          # Linux scroll up
                canvas.yview_scroll(-1, "units")
            elif event.num == 5:        # Linux scroll down
                canvas.yview_scroll(1, "units")
            else:                        # Windows / macOS
                canvas.yview_scroll(-1 if event.delta > 0 else 1, "units")
        canvas.bind_all("<MouseWheel>", _on_mousewheel)
        canvas.bind_all("<Button-4>", _on_mousewheel)
        canvas.bind_all("<Button-5>", _on_mousewheel)

        canvas.pack(side="left", fill="both", expand=True)
        scrollbar.pack(side="right", fill="y")


class ChunkEditorFrame(ttk.LabelFrame):
    """One editable form for a single chunk type (JDYN or THRS)."""

    def __init__(self, parent, title, fields):
        super().__init__(parent, text=title, padding=8)
        self.fields = fields
        self.vars = {}
        self.chunk = None  # the located Chunk object, once a file is open

        header = ttk.Frame(self)
        header.grid(row=0, column=0, columnspan=4, sticky="w")
        for col, text in enumerate(["Offset", "Field", "Value", "Type"]):
            ttk.Label(header, text=text, font=("", 9, "bold")).grid(
                row=0, column=col, padx=4, sticky="w")

        body = ttk.Frame(self)
        body.grid(row=1, column=0, sticky="nsew")

        for i, (offset, size, ftype, name) in enumerate(fields):
            ttk.Label(body, text=f"{offset:#04x}").grid(row=i, column=0, padx=4, sticky="w")
            ttk.Label(body, text=name).grid(row=i, column=1, padx=4, sticky="w")
            var = tk.StringVar(value="")
            entry = ttk.Entry(body, textvariable=var, width=14)
            entry.grid(row=i, column=2, padx=4, pady=1, sticky="w")
            ttk.Label(body, text=ftype, foreground="#666").grid(row=i, column=3, padx=4, sticky="w")
            self.vars[name] = var

        self.status = ttk.Label(self, text="No file loaded", foreground="#888")
        self.status.grid(row=2, column=0, sticky="w", pady=(6, 0))

    def load(self, chunk):
        """Populate the form from a located chunk's raw bytes."""
        self.chunk = chunk
        if chunk is None:
            for var in self.vars.values():
                var.set("")
            self.status.config(text="Chunk not found in this file")
            return
        for offset, size, ftype, name in self.fields:
            value = read_field(chunk.data, offset, size, ftype)
            text = f"{value:.4f}" if ftype == "fixed24.8" else str(value)
            self.vars[name].set(text)
        self.status.config(text=f"Loaded ({len(chunk.data)} bytes)")

    def collect_bytes(self):
        """Re-encode all current field values into a single bytes
        object matching the chunk's expected size. Raises ValueError
        with a field-specific message on bad input."""
        if self.chunk is None:
            return None
        out = bytearray(len(self.chunk.data))
        for offset, size, ftype, name in self.fields:
            text = self.vars[name].get().strip()
            try:
                if ftype == "fixed24.8":
                    value = float(text)
                else:
                    value = int(text, 0)  # allows 0x.. entry too
                    lo, hi = field_range(size, ftype)
                    if not (lo <= value <= hi):
                        raise ValueError(f"{name}: {value} out of range [{lo}, {hi}]")
                raw = write_field(value, size, ftype)
            except ValueError as e:
                if "out of range" in str(e):
                    raise
                raise ValueError(f"{name}: invalid value {text!r}")
            out[offset:offset + size] = raw
        return bytes(out)


class JdynEditorApp:
    def __init__(self, root, initial_path=None):
        self.root = root
        self.path = None
        self.file_bytes = None

        root.title("JDYN / THRS Editor")
        root.geometry("560x760")

        self._build_menu()

        container = ttk.Frame(root, padding=8)
        container.pack(fill="both", expand=True)

        toolbar = ttk.Frame(container)
        toolbar.pack(fill="x", pady=(0, 8))
        ttk.Button(toolbar, text="Open...", command=self.open_dialog).pack(side="left", padx=(0, 4))
        ttk.Button(toolbar, text="Save", command=self.save_file).pack(side="left", padx=4)
        ttk.Button(toolbar, text="Save As...", command=self.save_as_dialog).pack(side="left", padx=4)

        self.path_label = ttk.Label(container, text="(no file open)", foreground="#444")
        self.path_label.pack(anchor="w", pady=(0, 8))

        scroll_area = ScrollableFrame(container)
        scroll_area.pack(fill="both", expand=True)
        content = scroll_area.inner

        self.jdyn_frame = ChunkEditorFrame(content, "JDYN — Flight Dynamics", JDYN_FIELDS)
        self.jdyn_frame.pack(fill="x", pady=(0, 8))

        self.thrs_frame = ChunkEditorFrame(content, "THRS — Thrust", THRS_FIELDS)
        self.thrs_frame.pack(fill="x")

        self.chunk_selectors = {}  # tag -> (Combobox, list of Chunk)
        self._build_object_pickers(content)

        if initial_path:
            self.open_file(initial_path)

    # -- menu -----------------------------------------------------------

    def _build_menu(self):
        menubar = tk.Menu(self.root)
        filemenu = tk.Menu(menubar, tearoff=0)
        filemenu.add_command(label="Open...", command=self.open_dialog, accelerator="Ctrl+O")
        filemenu.add_command(label="Save", command=self.save_file, accelerator="Ctrl+S")
        filemenu.add_command(label="Save As...", command=self.save_as_dialog)
        filemenu.add_separator()
        filemenu.add_command(label="Quit", command=self.root.quit)
        menubar.add_cascade(label="File", menu=filemenu)
        self.root.config(menu=menubar)

        self.root.bind_all("<Control-o>", lambda e: self.open_dialog())
        self.root.bind_all("<Control-s>", lambda e: self.save_file())
        self.root.bind_all("<Command-o>", lambda e: self.open_dialog())
        self.root.bind_all("<Command-s>", lambda e: self.save_file())

    def _build_object_pickers(self, parent):
        """If a file contains more than one JDYN or THRS chunk (more
        than one aircraft/object), let the user pick which one to edit."""
        frame = ttk.Frame(parent)
        # placed above the chunk frames; kept simple, only shown if needed
        self.picker_frame = frame

    # -- file operations --------------------------------------------------

    def open_dialog(self):
        path = filedialog.askopenfilename(
            title="Open .IFF jet file",
            filetypes=[("IFF jet files", "*.iff *.IFF"), ("All files", "*.*")])
        if path:
            self.open_file(path)

    def open_file(self, path):
        try:
            with open(path, "rb") as f:
                data = f.read()
        except OSError as e:
            messagebox.showerror("Open failed", str(e))
            return

        jdyn_chunks = find_all(data, "JDYN")
        thrs_chunks = find_all(data, "THRS")

        if not jdyn_chunks and not thrs_chunks:
            messagebox.showwarning("No data", "No JDYN or THRS chunk found in this file.")

        self.path = path
        self.file_bytes = bytearray(data)
        self.path_label.config(text=path)

        # if multiple objects, just take the first of each for now
        # (typical Strike Commander jet files only have one)
        self.jdyn_frame.load(jdyn_chunks[0] if jdyn_chunks else None)
        self.thrs_frame.load(thrs_chunks[0] if thrs_chunks else None)

        if len(jdyn_chunks) > 1 or len(thrs_chunks) > 1:
            messagebox.showinfo(
                "Multiple objects",
                f"This file has {len(jdyn_chunks)} JDYN and {len(thrs_chunks)} THRS "
                "chunks. Only the first of each is shown/edited.")

    def _apply_edits_to_buffer(self):
        """Write the current form values back into self.file_bytes.
        Returns True on success, False (with a message shown) on bad input."""
        if self.file_bytes is None:
            messagebox.showwarning("No file", "Open a file first.")
            return False
        try:
            jdyn_bytes = self.jdyn_frame.collect_bytes()
            thrs_bytes = self.thrs_frame.collect_bytes()
        except ValueError as e:
            messagebox.showerror("Invalid value", str(e))
            return False

        if jdyn_bytes is not None:
            off = self.jdyn_frame.chunk.data_offset
            self.file_bytes[off:off + len(jdyn_bytes)] = jdyn_bytes
        if thrs_bytes is not None:
            off = self.thrs_frame.chunk.data_offset
            self.file_bytes[off:off + len(thrs_bytes)] = thrs_bytes
        return True

    def save_file(self):
        if self.path is None:
            self.save_as_dialog()
            return
        if not self._apply_edits_to_buffer():
            return
        try:
            with open(self.path, "wb") as f:
                f.write(self.file_bytes)
        except OSError as e:
            messagebox.showerror("Save failed", str(e))
            return
        messagebox.showinfo("Saved", f"Saved to {self.path}")

    def save_as_dialog(self):
        if self.file_bytes is None:
            messagebox.showwarning("No file", "Open a file first.")
            return
        if not self._apply_edits_to_buffer():
            return
        path = filedialog.asksaveasfilename(
            title="Save .IFF jet file as",
            defaultextension=".IFF",
            filetypes=[("IFF jet files", "*.iff *.IFF"), ("All files", "*.*")])
        if not path:
            return
        try:
            with open(path, "wb") as f:
                f.write(self.file_bytes)
        except OSError as e:
            messagebox.showerror("Save failed", str(e))
            return
        self.path = path
        self.path_label.config(text=path)
        messagebox.showinfo("Saved", f"Saved to {path}")


def main():
    initial_path = sys.argv[1] if len(sys.argv) > 1 else None
    root = tk.Tk()
    JdynEditorApp(root, initial_path)
    root.mainloop()


if __name__ == "__main__":
    main()
