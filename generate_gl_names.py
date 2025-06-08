#!/usr/bin/env python3
import re
import sys

def parse_defines(header_path):
    """
    Parse #define lines and build a map:
      GLenum_name -> integer value
    Then invert to: value -> list of names.
    """
    name_to_val = {}
    define_re = re.compile(r'#define\s+(\w+)\s+(.+)')
    with open(header_path, 'r', encoding='utf-8') as f:
        for line in f:
            m = define_re.match(line)
            if not m:
                continue
            name, val_str = m.groups()
            val_str = val_str.split()[0].rstrip('uUlL')
            try:
                val = int(val_str, 0)
            except ValueError:
                continue
            name_to_val[name] = val

    # invert to value -> [names]
    value_to_names = {}
    for name, val in name_to_val.items():
        value_to_names.setdefault(val, []).append(name)
    return value_to_names

def generate_c_header(mapping, out_path, guard='GLAD_ENUM_NAME_MAP_H'):
    """
    Generate a C header with:
      static inline const char *glad_enum_name(GLenum v);
    """
    with open(out_path, 'w', encoding='utf-8') as out:
        out.write(f"#ifndef {guard}\n#define {guard}\n\n")
        out.write("#include <GL/gl.h>  // or your GL include\n\n")
        out.write("static inline const char *glad_enum_name(GLenum value) {\n")
        out.write("    switch (value) {\n")
        for val, names in sorted(mapping.items(), key=lambda kv: kv[0]):
            # use the first name as the switch label
            label = names[0]
            # build the returned string
            if len(names) == 1:
                ret = f"\"{names[0]}\""
            else:
                joined = ", ".join(names)
                ret = f"\"[{joined}]\""
            out.write(f"        case {label}:\n")
            out.write(f"            return {ret};\n")
        out.write("        default:\n")
        out.write("            return NULL;  // unknown enum value\n")
        out.write("    }\n")
        out.write("}\n\n")
        out.write(f"#endif // {guard}\n")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python gen_glad_enum_map.py <path/to/glad.h> <output_header.h>")
        sys.exit(1)
    input_h, output_h = sys.argv[1], sys.argv[2]
    mapping = parse_defines(input_h)
    generate_c_header(mapping, output_h)
    print(f"Generated '{output_h}' with {len(mapping)} enum entries.")
