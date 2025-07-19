import os
import struct
import argparse
from Crypto.Cipher import AES
from Crypto.Util.Padding import unpad

def unpack_resource_pack(pack_file, output_dir, password):
    # Prepare password (pad to 32 bytes or truncate)
    password_bytes = password.encode('utf-8')
    if len(password_bytes) > 32:
        password_bytes = password_bytes[:32]
    else:
        password_bytes = password_bytes.ljust(32, b'\0')

    # Read the entire resource pack
    with open(pack_file, 'rb') as f:
        pack_data = f.read()

    # Validate header
    if pack_data[:4] != b'RPCK':
        print("Error: Invalid resource pack format (missing RPCK header)")
        return False

    # Read index size
    index_size = struct.unpack('<I', pack_data[4:8])[0]
    if index_size == 0 or 8 + index_size > len(pack_data):
        print("Error: Invalid index size")
        return False

    # Decrypt index
    iv = b'\0' * 16  # Zero IV to match encryption
    cipher = AES.new(password_bytes, AES.MODE_CBC, iv)
    encrypted_index = pack_data[8:8 + index_size]
    try:
        decrypted_index_padded = cipher.decrypt(encrypted_index)
        decrypted_index = unpad(decrypted_index_padded, AES.block_size)
    except ValueError as e:
        print(f"Error: Decryption failed - {e}")
        return False

    # Parse index (256-byte filename + 12 bytes FileEntry: offset, enc_size, original_size)
    index = []
    offset = 0
    while offset + 256 + 12 <= len(decrypted_index):
        filename = decrypted_index[offset:offset + 256].rstrip(b'\0').decode('utf-8')
        offset += 256
        file_entry = struct.unpack('<III', decrypted_index[offset:offset + 12])
        offset += 12
        index.append((filename, file_entry[0], file_entry[1], file_entry[2]))

    # Create output directory if it doesn't exist
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    # Decrypt and extract files
    for filename, file_offset, enc_size, original_size in index:
        data_offset = 8 + index_size + file_offset
        if data_offset + enc_size > len(pack_data):
            print(f"Error: Invalid data offset for {filename}")
            continue

        encrypted_data = pack_data[data_offset:data_offset + enc_size]
        cipher = AES.new(password_bytes, AES.MODE_CBC, iv)
        try:
            decrypted_data_padded = cipher.decrypt(encrypted_data)
            decrypted_data = unpad(decrypted_data_padded, AES.block_size)[:original_size]
        except ValueError as e:
            print(f"Error: Failed to decrypt {filename} - {e}")
            continue

        # Print file info
        print(f"[UNPACK] {filename} -> offset={file_offset}, enc={enc_size}, raw={original_size}")

        # Save file to output directory
        output_path = os.path.join(output_dir, filename)
        with open(output_path, 'wb') as f:
            f.write(decrypted_data)

    print(f"[DONE] Unpacked {len(index)} files from '{pack_file}'.")
    return True

def main():
    parser = argparse.ArgumentParser(description='Unpack an encrypted resource pack.')
    parser.add_argument('pack_file', help='Input resource pack file')
    parser.add_argument('output_dir', help='Output directory to extract files')
    parser.add_argument('password', help='Decryption password')
    args = parser.parse_args()

    unpack_resource_pack(args.pack_file, args.output_dir, args.password)

if __name__ == '__main__':
    main()
