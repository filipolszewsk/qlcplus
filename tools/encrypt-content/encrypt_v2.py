#!/usr/bin/env python3
"""
Encrypt a .js script into .qlcscript using the key from license.qlckey.
Implements the same logic as QLCCrypto (Qt C++) in pure Python.

Requirements: pip install pycryptodome
"""

import subprocess
import hashlib
import json
import struct
import os
import sys
from Crypto.Cipher import AES

BLOCK_LEN = 16
KEY_LEN   = 32
MAGIC     = b"QLCP"
VERSION   = b"\x01"


def get_hardware_fingerprint() -> str:
    """Replicate QLCCrypto::generateHardwareFingerprint() on macOS."""
    data = ""

    # IOPlatformUUID (stable, board-level)
    try:
        out = subprocess.check_output(
            ["ioreg", "-rd1", "-c", "IOPlatformExpertDevice"],
            stderr=subprocess.DEVNULL
        ).decode("utf-8", errors="replace")
        for line in out.splitlines():
            if "IOPlatformUUID" in line:
                # line looks like:  "IOPlatformUUID" = "XXXXXXXX-..."
                uuid = line.split("=", 1)[1].strip().strip('"')
                data += uuid
                break
    except Exception as e:
        print(f"[warn] ioreg failed: {e}", file=sys.stderr)

    # Root disk device path  (e.g. /dev/disk3s1s1)
    try:
        out = subprocess.check_output(
            ["df", "/"],
            stderr=subprocess.DEVNULL
        ).decode("utf-8", errors="replace")
        lines = out.strip().splitlines()
        if len(lines) >= 2:
            device = lines[-1].split()[0]   # first column = device
            data += device
    except Exception as e:
        print(f"[warn] df failed: {e}", file=sys.stderr)

    fingerprint_bytes = hashlib.sha256(data.encode("utf-8")).digest()
    return fingerprint_bytes.hex()   # hex string, as Qt's .toHex() returns


def derive_key(fingerprint_hex: str) -> bytes:
    """QLCCrypto::deriveKey(fingerprint) = SHA256(fingerprint.toUtf8())"""
    return hashlib.sha256(fingerprint_hex.encode("utf-8")).digest()


def pkcs7_unpad(data: bytes) -> bytes:
    if not data:
        return data
    pad = data[-1]
    if pad < 1 or pad > BLOCK_LEN:
        raise ValueError(f"Invalid PKCS7 padding byte: {pad}")
    for b in data[-pad:]:
        if b != pad:
            raise ValueError("Invalid PKCS7 padding")
    return data[:-pad]


def pkcs7_pad(data: bytes) -> bytes:
    pad = BLOCK_LEN - (len(data) % BLOCK_LEN)
    return data + bytes([pad] * pad)


def aes_decrypt(ciphertext: bytes, key: bytes) -> bytes:
    if len(ciphertext) < BLOCK_LEN * 2:
        raise ValueError("Ciphertext too short")
    iv        = ciphertext[:BLOCK_LEN]
    encrypted = ciphertext[BLOCK_LEN:]
    cipher    = AES.new(key, AES.MODE_CBC, iv)
    decrypted = cipher.decrypt(encrypted)
    return pkcs7_unpad(decrypted)


def aes_encrypt(plaintext: bytes, key: bytes) -> bytes:
    iv      = os.urandom(BLOCK_LEN)
    padded  = pkcs7_pad(plaintext)
    cipher  = AES.new(key, AES.MODE_CBC, iv)
    return iv + cipher.encrypt(padded)


def load_content_key(license_path: str) -> bytes:
    fingerprint = get_hardware_fingerprint()
    print(f"[info] Hardware fingerprint: {fingerprint[:16]}...", file=sys.stderr)

    hw_key = derive_key(fingerprint)

    with open(license_path, "rb") as f:
        encrypted_license = f.read()

    try:
        decrypted = aes_decrypt(encrypted_license, hw_key)
    except Exception:
        decrypted = b""

    if not decrypted:
        # try legacy fingerprint (with hostname)
        hostname = subprocess.check_output(["hostname"], stderr=subprocess.DEVNULL).decode().strip()
        try:
            uuid_out = subprocess.check_output(
                ["ioreg", "-rd1", "-c", "IOPlatformExpertDevice"],
                stderr=subprocess.DEVNULL
            ).decode("utf-8", errors="replace")
            uuid = ""
            for line in uuid_out.splitlines():
                if "IOPlatformUUID" in line:
                    uuid = line.split("=", 1)[1].strip().strip('"')
                    break
        except Exception:
            uuid = ""
        df_out = subprocess.check_output(["df", "/"], stderr=subprocess.DEVNULL).decode()
        device = df_out.strip().splitlines()[-1].split()[0]
        legacy_data = uuid + hostname + device
        legacy_fp   = hashlib.sha256(legacy_data.encode("utf-8")).digest().hex()
        legacy_key  = derive_key(legacy_fp)
        try:
            decrypted = aes_decrypt(encrypted_license, legacy_key)
        except Exception:
            decrypted = b""

    if not decrypted:
        raise RuntimeError("Cannot decrypt license file — hardware mismatch?")

    obj = json.loads(decrypted.decode("utf-8"))
    if obj.get("magic") != "QLCPLUS_LICENSE":
        raise RuntimeError("Invalid license magic")

    content_key_hex = obj["content_key"]
    content_key     = bytes.fromhex(content_key_hex)
    if len(content_key) != KEY_LEN:
        raise RuntimeError(f"Invalid content key length: {len(content_key)}")

    print(f"[info] Content key loaded (first 8 bytes): {content_key.hex()[:16]}...", file=sys.stderr)
    return content_key


def encrypt_script(js_path: str, qlcscript_path: str, content_key: bytes):
    with open(js_path, "rb") as f:
        plaintext = f.read()

    encrypted = aes_encrypt(plaintext, content_key)
    output    = MAGIC + VERSION + encrypted

    with open(qlcscript_path, "wb") as f:
        f.write(output)

    print(f"[ok] {os.path.basename(js_path)} → {os.path.basename(qlcscript_path)} ({len(output)} bytes)")


if __name__ == "__main__":
    license_path   = os.path.expanduser("~/Library/Application Support/QLC+/license.qlckey")
    scripts_dir    = os.path.expanduser("~/Library/Application Support/QLC+/RGBScripts")
    decoded_dir    = os.path.join(scripts_dir, "_decoded")

    js_path        = os.path.join(decoded_dir,  "GRIDqlc_Crossfade_Toggle_v6.js")
    qlcscript_path = os.path.join(scripts_dir,  "GRIDqlc_Crossfade_Toggle_v6.qlcscript")

    print("[step 1] Loading content key from license...")
    key = load_content_key(license_path)

    print("[step 2] Encrypting script...")
    encrypt_script(js_path, qlcscript_path, key)

    print("[done]")
