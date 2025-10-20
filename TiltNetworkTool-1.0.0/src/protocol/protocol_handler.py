from typing import List, Dict, Any
import logging

class ProtocolHandler:
    """
    Clase intermedia para manejar el protocolo de comunicación serial.

    Qué se debe implementar:
      - Framing y parseo en on_bytes(): acumular bytes, detectar fin de mensaje,
        validar y convertir a una estructura uniforme para la GUI según lo especificado.
      - Construcción de mensajes salientes en build_led_command().
    """
    def __init__(self) -> None:
        # Buffer/s, constantes, ...        
        logging.info("[ProtocolHandler] Inicializado. Listo para recibir bytes del puerto serie.")
        self.buffer = str()
        self.frame_start = False
        self.current_station_index = None
        self.current_data_prefix = None
        self.current_data_value = None

    def on_bytes(self, data: bytes) -> List[Dict[str, Any]]:
        """
        Recibe bytes crudos desde el puerto serie y devuelve a la GUI una lista de mensajes parseados.

        Debe devolver: lista de mensajes. Cada mensaje es un dict con:
          - 'station_index': int (0..N-1)
          - 'angle': int en {0: roll, 1: pitch, 2: yaw}
          - 'value': float|int

        Ejemplos de retorno:
          # Un solo mensaje:
          # return [{'station_index': 0, 'angle': 0, 'value': 12.5}]  # roll de grupo 0
          # Varios mensajes:
          # return [
          #   {'station_index': 1, 'angle': 1, 'value': 20.0},  # pitch de grupo 1
          #   {'station_index': 1, 'angle': 2, 'value': -7.2},  # yaw de grupo 1
          # ]
          # Lista vacía si no hay frames completos:
          # return []
        """
        # Descripcion del protocolo:
        # Byte 0 - Start of Message: 0xAA
        # Byte 1 - Station Index: 
        # Bit 9+ Data - Prefix + Value: R/G/B + 0/1 for LED style commands or Y/P/T + 4 byte float for (yaw/pitch/roll) rotation data
        # Last Bit sequence: 11111111 indicates end of message (0xFF)
        self.buffer += data.decode('ascii', errors='ignore')
        logging.info(f"[ProtocolHandler] on_bytes() recibido {len(data)} bytes: {data}.")
        if len(self.buffer) > 1024:
            logging.warning("[ProtocolHandler] Buffer overflow. Limpiando buffer.")
            self.buffer = str()
        result = []
        while len(self.buffer) > 0:
            if self.frame_start is False:
                index = self.buffer.find('Begin:')
                if index == -1:
                    self.buffer = str()
                    return result
                self.buffer = self.buffer[index+6:]
                if len(self.buffer) == 0:
                    return result
                self.frame_start = True
            if self.current_station_index is None:
                index = self.buffer.find('Group:')
                if index == -1:
                    return result
                self.buffer = self.buffer[index+6:]
                self.current_station_index = int(self.buffer[0:1])
                self.buffer = self.buffer[1:]
                if len(self.buffer) == 0:
                    return result
            if self.current_data_prefix is None:
                self.current_data_prefix = self.buffer[0:1]
                self.buffer = self.buffer[2:]
            if self.current_data_value is None:
                if self.current_data_prefix in ['R', 'G', 'B']:
                    self.current_data_value = bool(int(self.buffer[0:1]))
                    self.buffer = self.buffer[1:]
                elif self.current_data_prefix in ['Y', 'P', 'T']:
                    if len(self.buffer) < 5:
                        return result
                    self.current_data_value = float(self.buffer[:6])
                    self.buffer = self.buffer[6:]
            if self.frame_start is True and self.current_station_index is not None and self.current_data_prefix is not None and self.current_data_value is not None:
                angle_map = {'Y': 2, 'P': 1, 'T': 0}
                message = {
                    'station_index': self.current_station_index,
                    'angle': angle_map.get(self.current_data_prefix, -1),
                    'value': self.current_data_value
                  }
                result.append(message)
                self.frame_start = False
                self.current_station_index = None
                self.current_data_prefix = None
                self.current_data_value = None

        return result
        

    def build_led_command(self, station_index: int, r: bool, g: bool, b: bool) -> bytes:
        """
        Construye los bytes a enviar por serial para comandar LEDs de una estación.

        Parámetros:
          - station_index: int (0..N-1)
          - r, g, b: bools que indican encendido de cada color

        Debe devolver:
          - bytes listos para write() del puerto serie.
        """
        logging.info(f"[ProtocolHandler] Build LED cmd -> station={station_index}, R={r}, G={g}, B={b}")

        #
        #
        #
        
        # return b'Prender LED rojo del grupo 5'

        raise NotImplementedError("Implementar build_led_command(...) para su protocolo.")
