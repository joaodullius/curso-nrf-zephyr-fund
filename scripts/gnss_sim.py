
import random
import datetime
import serial
import time
import argparse

def decimal_to_nmea(degrees, is_latitude):
    degrees_abs = abs(degrees)
    d = int(degrees_abs)
    m = (degrees_abs - d) * 100
    if is_latitude:
        hemi = 'N' if degrees >= 0 else 'S'
        return f"{d:02d}{m:07.4f}", hemi
    else:
        hemi = 'E' if degrees >= 0 else 'W'
        return f"{d:03d}{m:07.4f}", hemi

def calculate_checksum(nmea_str):
    cs = 0
    for c in nmea_str:
        cs ^= ord(c)
    return f"{cs:02X}"

def generate_gga(lat, lon, alt):
    now = datetime.datetime.now(datetime.UTC)
    time_str = now.strftime("%H%M%S.00")
    lat_str, lat_hemi = decimal_to_nmea(lat, is_latitude=True)
    lon_str, lon_hemi = decimal_to_nmea(lon, is_latitude=False)
    fix_quality = 1
    num_sats = random.randint(7, 12)
    hdop = round(random.uniform(0.6, 1.5), 1)
    geoid_sep = -3.0
    gga_core = f"GNGGA,{time_str},{lat_str},{lat_hemi},{lon_str},{lon_hemi},{fix_quality},{num_sats},{hdop:.1f},{alt:.1f},M,{geoid_sep:.1f},M,,"
    checksum = calculate_checksum(gga_core)
    return f"${gga_core}*{checksum}"

def generate_gsv():
    num_sats = random.randint(4, 7)
    total_msgs = (num_sats + 3) // 4
    gsv_msgs = []
    prns = random.sample(range(1, 33), num_sats)

    for msg_num in range(1, total_msgs + 1):
        sats_in_msg = prns[(msg_num - 1)*4 : msg_num*4]
        msg_fields = [f"GNGSV,{total_msgs},{msg_num},{num_sats}"]
        for prn in sats_in_msg:
            elev = random.randint(5, 85)
            azim = random.randint(0, 359)
            snr = random.randint(25, 48)
            msg_fields.extend([f"{prn:02d}", f"{elev:02d}", f"{azim:03d}", f"{snr:02d}"])
        while len(msg_fields) < 19:
            msg_fields.append('')
        core = ",".join(msg_fields)
        checksum = calculate_checksum(core)
        gsv_msgs.append(f"${core}*{checksum}")
    return gsv_msgs

def apply_position_jitter(lat, lon, alt):
    jitter = lambda x: x + random.uniform(-0.000009, 0.000009)
    return jitter(lat), jitter(lon), alt + random.uniform(-1, 1)

def main():
    parser = argparse.ArgumentParser(description="Simulador GNSS UART com opção de leitura")
    parser.add_argument("--com", required=True, help="Porta serial COM (ex: COM3 ou /dev/ttyUSB0)")
    parser.add_argument("--rate", type=int, default=115200, help="Baudrate (default: 115200)")
    parser.add_argument("--read", action="store_true", help="Ativa leitura da resposta da placa")
    args = parser.parse_args()

    port = args.com
    baudrate = args.rate
    do_read = args.read
    interval = 1.0  # envio a cada 1 segundo

    lat_ref = -23.585099907083166
    lon_ref = -46.63520638026592
    alt_ref = 760.0


    with serial.Serial(port, baudrate, timeout=0.1) as ser:
        while True:
            lat, lon, alt = apply_position_jitter(lat_ref, lon_ref, alt_ref)
            gga = generate_gga(lat, lon, alt)
            gsv_msgs = generate_gsv()

            ser.write((gga + '\r\n').encode('ascii'))
            for gsv in gsv_msgs:
                ser.write((gsv + '\r\n').encode('ascii'))

            if do_read:
                incoming = ser.read(ser.in_waiting or 1)
                if incoming:
                    print(incoming.decode('ascii', errors='replace'), end='')

            time.sleep(interval)

if __name__ == "__main__":
    main()
