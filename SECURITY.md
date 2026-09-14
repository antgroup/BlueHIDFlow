# Security Policy

## Reporting Security Vulnerabilities

**Please do not report security vulnerabilities through public GitHub issues.**

If you discover a security vulnerability in BlueHIDFlow, please report it responsibly:

1. **Email**: Send a detailed report to the project maintainers
2. **GitHub Security Advisory**: Use GitHub's private vulnerability reporting feature

### What to Include

- Description of the vulnerability
- Steps to reproduce
- Potential impact
- Suggested fix (if available)

## Security Considerations

### MQTT Communication

- BlueHIDFlow supports TLS-encrypted MQTT connections (port 8883)
- **Always use TLS** in production environments
- MQTT credentials are stored in ESP32 NVS flash — physical access to the device can extract them
- Use strong, unique credentials for your MQTT broker

### WiFi Configuration

- AP mode password is configurable through the Web portal
- Change the default AP password before deploying
- WiFi credentials are stored in NVS flash

### BLE HID

- BLE HID connections allow control of paired devices
- Only pair with trusted devices
- Unpair devices when not in use to prevent unauthorized control

### Device Security

- The ESP32-S3 does not have secure boot enabled by default
- Firmware can be flashed via USB without authentication
- Consider enabling flash encryption for production deployments
- Physical access to the device allows full firmware replacement

## Supported Versions

| Version | Supported |
| ------- | --------- |
| 0.9.x   | Yes       |
| < 0.9   | No        |

## Best Practices

1. **Change default credentials** — Always change MQTT and AP passwords from defaults
2. **Use TLS** — Enable MQTT TLS (port 8883) for encrypted communication
3. **Network isolation** — Place IoT devices on a separate VLAN when possible
4. **Regular updates** — Keep firmware up to date with the latest security patches
5. **Access control** — Restrict MQTT broker access with proper authentication and ACLs
