import serial
import time

# Configurar a porta serial (ajuste a velocidade se necessário)
ser = serial.Serial('/dev/ttyUSB0', baudrate=115200, timeout=1)

# Abrir o arquivo log.txt em modo de adição
with open('log.txt', 'a') as log_file:
    print("Lendo dados da serial e salvando em log.txt...")
    
    try:
        while True:
            if ser.in_waiting > 0:  # Verificar se há dados disponíveis na porta serial
                data = ser.readline().decode('utf-8').strip()  # Ler linha da porta serial e decodificar
                print(data)  # Opcional: Exibir os dados no terminal
                log_file.write(f"{time.strftime('%Y-%m-%d %H:%M:%S')} - {data}\n")  # Escrever dados no arquivo com timestamp
                log_file.flush()  # Garantir que os dados sejam gravados imediatamente

    except KeyboardInterrupt:
        print("Leitura interrompida")

    finally:
        ser.close()  # Fechar a porta serial ao finalizar
