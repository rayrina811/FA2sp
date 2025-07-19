import os
import struct
import argparse
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad

def pack_directory(input_dir, output_file, password):
    # Prepare password (pad to 32 bytes or truncate)
    password_bytes = password.encode('utf-8')
    if len(password_bytes) > 32:
        password_bytes = password_bytes[:32]
    else:
        password_bytes = password_bytes.ljust(32, b'\0')
    
    # Initialize data structures
    file_data = []
    index_entries = []
    current_offset = 0
    
    # Process all files in directory
    for filename in sorted(os.listdir(input_dir)):
        filepath = os.path.join(input_dir, filename)
        if os.path.isfile(filepath):
            with open(filepath, 'rb') as f:
                raw_data = f.read()
            
            # Pad data to AES block size (16 bytes)
            padded_data = pad(raw_data, AES.block_size)
            
            # Encrypt data
            iv = b'\0' * 16  # Zero IV for consistency with decryption
            cipher = AES.new(password_bytes, AES.MODE_CBC, iv)
            encrypted_data = cipher.encrypt(padded_data)
            
            # Store file information
            file_data.append(encrypted_data)
            
            # Create index entry (256-byte filename + FileEntry struct)
            filename_padded = filename.encode('utf-8').ljust(256, b'\0')
            index_entry = filename_padded + struct.pack('<III', current_offset, len(encrypted_data), len(raw_data))
            index_entries.append(index_entry)
            
            # Print packing info
            print(f"[PACK] {filename} -> offset={current_offset}, enc={len(encrypted_data)}, raw={len(raw_data)}")
            
            current_offset += len(encrypted_data)
    
    # Create index table
    index_data = b''.join(index_entries)
    index_cipher = AES.new(password_bytes, AES.MODE_CBC, iv)
    encrypted_index = index_cipher.encrypt(pad(index_data, AES.block_size))
    
    # Write output file
    with open(output_file, 'wb') as f:
        # Write header
        f.write(b'RPCK')
        f.write(struct.pack('<I', len(encrypted_index)))
        f.write(encrypted_index)
        for data in file_data:
            f.write(data)
    
    print(f"[DONE] Packed {len(file_data)} files into '{output_file}'.")
    print(f"Actual password used: {password_bytes.decode('utf-8', errors='ignore')}")
    
def main():
    parser = argparse.ArgumentParser(description='Pack files into an encrypted resource pack.')
    parser.add_argument('input_dir', help='Input directory containing files to pack')
    parser.add_argument('output_file', help='Output resource pack file')
    parser.add_argument('password', help='Encryption password')
    args = parser.parse_args()
    
    pack_directory(args.input_dir, args.output_file, args.password)

if __name__ == '__main__':
    main()
