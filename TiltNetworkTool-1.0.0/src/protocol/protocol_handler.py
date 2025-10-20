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
        self.buffer = bytearray()
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
        # Bit 0-8 - Station Index: 
        # Bit 9+ Data - Prefix + Value: R/G/B + 0/1 for LED style commands or Y/P/T + 4 byte float for (yaw/pitch/roll) rotation data
        # Last Bit sequence: 11111111 indicates end of message (0xFF)
        logging.info(f"[ProtocolHandler] on_bytes() recibido {len(data)} bytes: {data}.")
        result = []
        data = bytearray(data)
        while len(data) > 0:
            if self.current_station_index is None:
                if len(self.buffer) >= 1:
                    self.current_station_index = int.from_bytes(bytes(self.buffer[:1]), 'big')
                    self.buffer = self.buffer[1:]
                else:
                    self.buffer.extend(data[:1])
                    self.current_station_index = int.from_bytes(self.buffer, 'big')
                    data = data[1:]
                    self.buffer.clear()
            if self.current_data_prefix is None:
                self.data_prefix = data[0]
                data = data[1:]
            if self.current_data_value is None:
                if self.data_prefix in [b'R', b'G', b'B']:
                    self.current_data_value = bool(int.from_bytes(data[0:1], 'big'))
                    data = data[1:]
                elif self.data_prefix in [b'Y', b'P', b'T']:
                    if (len(self.buffer) + len(data)) >= 4:
                        if len(self.buffer) >= 4:
                            self.current_data_value = float.from_bytes(bytes(self.buffer[:4]))
                            self.buffer = self.buffer[4:]
                        else:
                            self.buffer.extend(data[:4 - len(self.buffer)])
                            self.current_data_value = float.from_bytes(bytes(self.buffer))
                            num_emplaced = 4 - len(self.buffer)
                            data = data[num_emplaced:]
                            self.buffer.clear()
                    else:
                        self.buffer.extend(data)
                        return []
            if self.current_station_index is not None and self.current_data_prefix is not None and self.current_data_value is not None:
                angle_map = {b'Y': 2, b'P': 1, b'T': 0}
                message = {
                    'station_index': self.current_station_index,
                    'angle': angle_map.get(self.data_prefix, -1),
                    'value': self.current_data_value
                  }
                result.append(message)
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
