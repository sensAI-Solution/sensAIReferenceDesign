# -----------------------------------------------------------------------------
# Copyright (c) 2025 Lattice Semiconductor Corporation
#
# SPDX-License-Identifier: UNLICENSED
#
# -----------------------------------------------------------------------------
"""
key_cert_generation.py

This script generates a self-signed SSL certificate and private key for local development or secure communication.
It automatically detects local IP addresses and adds them to the certificate's Subject Alternative Name (SAN) field,
so the certificate works for localhost and your machine's network IPs. The generated files are 'server.crt' and 'server.key'.

"""

# Using cryptography library to generate self-signed certificates and RSA (Rivest–Shamir–Adleman) cyber security algorithm for private keys.
from cryptography import x509
from cryptography.x509.oid import NameOID
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from datetime import datetime, timedelta, timezone
import ipaddress
# Using socket library to gather local IP addresses
import socket

def get_local_ip_addresses():
    """
    Gather all local IPv4 addresses for this machine using multiple methods.
    This helps ensure the certificate works for all network interfaces.
    """
    ip_addresses = []
    ip_addresses.append("127.0.0.1")  # Always include localhost
    try:
        # Get IP using hostname
        hostname = socket.gethostname()
        local_ip = socket.gethostbyname(hostname)
        if local_ip not in ip_addresses:
            ip_addresses.append(local_ip)
    except:
        pass

    try:
        # Get IP by connecting to a public server (works for most setups)
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            s.connect(("8.8.8.8", 80))
            local_ip = s.getsockname()[0]
            if local_ip not in ip_addresses:
                ip_addresses.append(local_ip)
    except:
        pass

    try:
        # Use system commands for more IPs (Windows, Linux, Mac)
        import subprocess
        import platform

        if platform.system() == "Windows":
            # Parse output of ipconfig for IPv4 addresses
            result = subprocess.run(['ipconfig'], capture_output=True, text=True, timeout=10)
            lines = result.stdout.split('\n')
            for line in lines:
                if 'IPv4 Address' in line or 'IP Address' in line:
                    ip = line.split(':')[-1].strip()
                    if ip and '.' in ip and ip not in ip_addresses:
                        try:
                            ipaddress.IPv4Address(ip)
                            ip_addresses.append(ip)
                        except:
                            pass
        else:
            # Try hostname -I (Linux)
            try:
                result = subprocess.run(['hostname', '-I'], capture_output=True, text=True, timeout=5)
                ips = result.stdout.strip().split()
                for ip in ips:
                    if ip and '.' in ip and ':' not in ip and ip not in ip_addresses:
                        try:
                            ipaddress.IPv4Address(ip)
                            ip_addresses.append(ip)
                        except:
                            pass
            except:
                # Fallback to ip addr show
                try:
                    result = subprocess.run(['ip', 'addr', 'show'], capture_output=True, text=True, timeout=5)
                    lines = result.stdout.split('\n')
                    for line in lines:
                        if 'inet ' in line and 'scope global' in line:
                            parts = line.strip().split()
                            for part in parts:
                                if '/' in part and '.' in part:
                                    ip = part.split('/')[0]
                                    if ip not in ip_addresses:
                                        try:
                                            ipaddress.IPv4Address(ip)
                                            ip_addresses.append(ip)
                                        except:
                                            pass
                except:
                    # Fallback to ifconfig
                    try:
                        result = subprocess.run(['ifconfig'], capture_output=True, text=True, timeout=5)
                        lines = result.stdout.split('\n')
                        for line in lines:
                            if 'inet ' in line and 'netmask' in line:
                                parts = line.strip().split()
                                if len(parts) >= 2:
                                    ip = parts[1]
                                    if ip not in ip_addresses:
                                        try:
                                            ipaddress.IPv4Address(ip)
                                            ip_addresses.append(ip)
                                        except:
                                            pass
                    except:
                        pass
    except:
        pass

    try:
        # Try socket.getaddrinfo for additional IPs
        for info in socket.getaddrinfo(socket.gethostname(), None, socket.AF_INET):
            ip = info[4][0]
            if ip not in ip_addresses:
                try:
                    ipaddress.IPv4Address(ip)
                    ip_addresses.append(ip)
                except:
                    pass
    except:
        pass

    return ip_addresses


key = rsa.generate_private_key(
    public_exponent=65537,
    key_size=4096,
)  # Generate a new RSA private key (4096 bits)

subject = issuer = x509.Name([
    # Certificate subject and issuer fields
    x509.NameAttribute(NameOID.COUNTRY_NAME, u"IN"),
    x509.NameAttribute(NameOID.STATE_OR_PROVINCE_NAME, u"Maharashtra"),
    x509.NameAttribute(NameOID.LOCALITY_NAME, u"Pune"),
    x509.NameAttribute(NameOID.ORGANIZATION_NAME, u"Lattice Semiconductor"),
    x509.NameAttribute(NameOID.ORGANIZATIONAL_UNIT_NAME, u"LPQ"),
    x509.NameAttribute(NameOID.COMMON_NAME, u"Lattice GARD-HUB UI"),
])

local_ips = get_local_ip_addresses()  # Get all local IP addresses for SAN field
print(f"Detected local IP addresses: {local_ips}")

# Add DNS names and IPs to SAN (Subject Alternative Name)
alt_names = [
    x509.DNSName(u"localhost"),  # Always allow localhost
    x509.DNSName(u"*.local"),    # Wildcard for .local domains
]

for ip_str in local_ips:
    try:
        ip_addr = ipaddress.IPv4Address(ip_str)
        alt_names.append(x509.IPAddress(ip_addr))
        print(f"Added IP address: {ip_str}")
    except ValueError:
        print(f"Skipping invalid IP: {ip_str}")

not_valid_before = datetime.now(tz=timezone.utc)
not_valid_after = not_valid_before + timedelta(days=3650)  # Valid for 10 years

# Certificate parameters including subject, issuer, validity period, SAN, etc.
cert = (
    x509.CertificateBuilder()
    .subject_name(subject)
    .issuer_name(issuer)
    .public_key(key.public_key())
    .serial_number(x509.random_serial_number())
    .not_valid_before(not_valid_before)
    .not_valid_after(not_valid_after)
    .add_extension(x509.SubjectAlternativeName(alt_names), critical=False)
    .sign(key, hashes.SHA256())
)  # Build and sign the certificate


with open("server.key", "wb") as f:
    # Save private key to file (PEM format, no password)
    f.write(
        key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.TraditionalOpenSSL,
            encryption_algorithm=serialization.NoEncryption(),
        )
    )


with open("server.crt", "wb") as f:
    # Save certificate to file (PEM format)
    f.write(cert.public_bytes(serialization.Encoding.PEM))


# Print success message for server key and certificate generation
print("\n" + "="*50)
print("Self-signed certificate and key generated successfully!")
print("Files created: server.crt, server.key")
print("="*50)
print("\nCertificate includes the following addresses:")
for alt_name in alt_names:
    if isinstance(alt_name, x509.DNSName):
        print(f"  DNS: {alt_name.value}")
    elif isinstance(alt_name, x509.IPAddress):
        print(f"  IP:  {alt_name.value}")
print("="*50)