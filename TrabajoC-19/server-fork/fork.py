#!/usr/bin/env python3
# tenedor.py

import socket
import struct
import threading
import time

MULTICAST_GROUP = '239.0.0.1'   # Debe coincidir con el grupo en C++
MULTICAST_PORT = 5353
BUFFER_SIZE = 512

class Tenedor:
    def __init__(self):
        # Lista de servidores conocidos: { 'ServidorA': ('IP', puerto) }
        self.servidores = {}
        self.lock = threading.Lock()
        # Socket para multicast
        self.mcast_sock = self._crear_socket_multicast()
        # Arrancamos hilos para escuchar anuncios y mensajes de muerte
        self.listening = True
        threading.Thread(target=self._escuchar_multicast, daemon=True).start()

    def _crear_socket_multicast(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        # Permitir usar la misma dirección multiple veces
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind(('', MULTICAST_PORT))  # escucha en cualquier interfaz
        # Unirse al grupo multicast
        mreq = struct.pack("4sl", socket.inet_aton(MULTICAST_GROUP), socket.INADDR_ANY)
        sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
        return sock

    def _escuchar_multicast(self):
        while self.listening:
            data, addr = self.mcast_sock.recvfrom(BUFFER_SIZE)
            mensaje = data.decode('utf-8').strip()
            if mensaje.startswith("ANUNCIO_SERVIDOR:"):
                # Formato esperado: "ANUNCIO_SERVIDOR:Nombre:IP:Puerto"
                partes = mensaje.split(':')
                if len(partes) == 4:
                    nombre, ip, puerto = partes[1], partes[2], int(partes[3])
                    with self.lock:
                        if nombre not in self.servidores:
                            self.servidores[nombre] = (ip, puerto)
                            print(f"[Tenedor] Descubierto servidor {nombre} en {ip}:{puerto}")
            elif mensaje.startswith("MORIRE:"):
                # Formato: "MORIRE:ServidorA"
                partes = mensaje.split(':')
                if len(partes) == 2:
                    nombre = partes[1]
                    with self.lock:
                        if nombre in self.servidores:
                            del self.servidores[nombre]
                            print(f"[Tenedor] Servidor {nombre} ha muerto y fue removido.")
            # Si recibimos otro tipo de mensaje, lo ignoramos

    def listar_servidores(self):
        with self.lock:
            return dict(self.servidores)

    def solicitar_figura(self, nombre_servidor, nombre_figura):
        """
        Se conecta por TCP al servidor (IP, puerto) para pedir la figura:
        GET /figure?name=nombre_figura HTTP/1.1
        """
        with self.lock:
            if nombre_servidor not in self.servidores:
                print(f"[Tenedor] Servidor {nombre_servidor} no está disponible.")
                return None
            ip, puerto = self.servidores[nombre_servidor]

        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect((ip, puerto))
            request = f"GET /figure?name={nombre_figura} HTTP/1.1\r\nHost: {ip}\r\n\r\n"
            s.sendall(request.encode('utf-8'))
            response = b''
            while True:
                parte = s.recv(BUFFER_SIZE)
                if not parte:
                    break
                response += parte
            s.close()
            return response.decode('utf-8')
        except Exception as e:
            print(f"[Tenedor] Error al conectar con {nombre_servidor}: {e}")
            return None

    def shutdown(self):
        self.listening = False
        self.mcast_sock.close()


if __name__ == "__main__":
    tenedor = Tenedor()
    try:
        while True:
            print("\n==== Menú Tenedor ====")
            print("1) Listar servidores disponibles")
            print("2) Solicitar figura a un servidor")
            print("3) Salir")
            opcion = input("Elige opción: ").strip()

            if opcion == '1':
                servs = tenedor.listar_servidores()
                if servs:
                    for nombre, (ip, puerto) in servs.items():
                        print(f"  - {nombre} -> {ip}:{puerto}")
                else:
                    print("  (No hay servidores descubiertos)")
            elif opcion == '2':
                nombre_srv = input("Nombre del servidor: ").strip()
                nombre_fig = input("Nombre de la figura: ").strip()
                resp = tenedor.solicitar_figura(nombre_srv, nombre_fig)
                if resp:
                    print("=== Respuesta HTTP ===")
                    print(resp)
                else:
                    print("No se recibió respuesta o hubo error.")
            elif opcion == '3':
                break
            else:
                print("Opción inválida.")
            time.sleep(0.2)
    except KeyboardInterrupt:
        pass
    finally:
        tenedor.shutdown()
        print("Tenedor finalizado.")
