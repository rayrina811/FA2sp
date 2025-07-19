import sys
import random
import textwrap

def generate_obfuscated_cpp(key):
    if len(key) != 32:
        raise ValueError("Key must be exactly 32 characters (256 bits)")

    key_bytes = [ord(c) for c in key]
    
    var_names = [f"v{i:02x}" for i in range(32)]
    func_names = [f"f{i:02x}" for i in range(8)]
    
    expressions = []
    for i, byte in enumerate(key_bytes):
        layers = random.randint(3, 6)
        masks = []
        
        total_xor = 0
        for j in range(layers - 1):
            mask = random.randint(1, 255)
            masks.append(mask)
            total_xor ^= mask
        
        final_mask = byte ^ total_xor
        masks.append(final_mask)
        
        expr_parts = [str(m) for m in masks]
        random.shuffle(expr_parts)
        expr = " ^ ".join(expr_parts)
        expressions.append(f"static_cast<unsigned char>({expr})")
    
    func_definitions = []
    for i in range(8):
        func_body = []
        func_body.append(f"unsigned char* {func_names[i]}() {{")
        func_body.append("    static unsigned char part[4];")
        
        for j in range(4):
            idx = i * 4 + j
            func_body.append(f"    unsigned char {var_names[idx]} = {expressions[idx]};")
            func_body.append(f"    part[{j}] = {var_names[idx]};")
        
        func_body.append("    return part;")
        func_body.append("}\n")
        func_definitions.append("\n".join(func_body))
    

    header = "Please copy & paste following code into \n.\\FA2sp\\Ext\\CLoading\\Body.ResourcePack.cpp -> namespace EncryptionKey\n\n"

    
    return header + "\n".join(func_definitions) 

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python generate_key_func.py <32_character_key>")
        sys.exit(1)
    
    key = sys.argv[1]
    try:
        cpp_code = generate_obfuscated_cpp(key)
        print(cpp_code)
    except ValueError as e:
        print(e)
        sys.exit(1)
