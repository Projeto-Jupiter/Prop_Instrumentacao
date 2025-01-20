import serial
import time
import sys

# Configurações da porta serial
SERIAL_PORT = '/dev/ttyUSB0'  # Atualize conforme necessário (ex: COM3 no Windows)
BAUD_RATE = 9600              # Deve corresponder à configuração do módulo E32

# Mapeamento das opções para códigos de três caracteres
OPCOES = {
    "1": {"descricao": "Ativar linha de abastecimento", "codigo": "ATL"},
    "2": {"descricao": "Desativar linha de abastecimento", "codigo": "DTL"},
    "3": {"descricao": "Fechar vent", "codigo": "FCV"},
    "4": {"descricao": "Ativar purga da linha", "codigo": "ATP"},
    "5": {"descricao": "Desativar purga da linha", "codigo": "DTP"},
    "6": {"descricao": "Ativar supercharge", "codigo": "ATS"},
    "7": {"descricao": "Desativar supercharge", "codigo": "DTS"},
    "8": {"descricao": "Ativar purga da linha", "codigo": "ATP"},
    "9": {"descricao": "Desativar purga da linha", "codigo": "DTP"},
    "10": {"descricao": "Ativar quick disconnect", "codigo": "ATQ"},
    "11": {"descricao": "Ativar ARM", "codigo": "ATA"},
    "12": {"descricao": "Ativar ignição", "codigo": "ATI"},
    "13": {"descricao": "Ativar injeção", "codigo": "ATJ"},
    "14": {"descricao": "Desativar ARM", "codigo": "DTA"},
    "15": {"descricao": "Desativar injeção", "codigo": "DTJ"},
    "16": {"descricao": "Desativar ignição", "codigo": "DTI"},
    "17": {"descricao": "Abrir vent", "codigo": "ABV"},
    "18": {"descricao": "Ativar purga do tanque", "codigo": "ATT"},
    "19": {"descricao": "Desativar purga do tanque", "codigo": "DTT"},
    "20": {"descricao": "Desativar quick disconnect", "codigo": "DTQ"},
}

# Separar as opções principais e complementares
OPCOES_PRINCIPAIS = {k: v for k, v in OPCOES.items() if 1 <= int(k) <= 13}
OPCOES_COMPLEMENTARES = {k: v for k, v in OPCOES.items() if 14 <= int(k) <= 20}

def exibir_menu():
    print("\nSelecione uma opção de atuação:\n")
    for numero in sorted(OPCOES_PRINCIPAIS.keys(), key=lambda x: int(x)):
        print(f"    {numero}. {OPCOES_PRINCIPAIS[numero]['descricao']}")
    
    print("\nOpções complementares:\n")
    for numero in sorted(OPCOES_COMPLEMENTARES.keys(), key=lambda x: int(x)):
        print(f"    {numero}. {OPCOES_COMPLEMENTARES[numero]['descricao']}")
    
    print("\nDigite 'sair' para encerrar o programa.")

def obter_selecao():
    while True:
        selecao = input("\nDigite o número da opção desejada: ").strip()
        if selecao.lower() == 'sair':
            return 'sair'
        if selecao in OPCOES:
            return selecao
        else:
            print("Opção inválida. Por favor, tente novamente.")

def confirmar_execucao(descricao):
    while True:
        confirmacao = input(f"Deseja executar a opção '{descricao}'? (sim/não): ").strip().lower()
        if confirmacao in ['sim', 'não', 's', 'n']:
            return confirmacao.startswith('s')
        else:
            print("Resposta inválida. Por favor, responda com 'sim' ou 'não'.")

def enviar_comando(ser, codigo):
    try:
        ser.write(codigo.encode('ascii'))
        ser.flush()
        print(f"Comando '{codigo}' enviado com sucesso.")
    except serial.SerialException as e:
        print(f"Erro ao enviar comando: {e}")

def esperar_confirmacao(ser, codigo_enviado, timeout=5):
    """
    Espera por uma confirmação do STM32 dentro do tempo limite especificado.
    Retorna True se a confirmação for recebida, False caso contrário.
    """
    print("Aguardando confirmação do receptor...")
    start_time = time.time()
    confirmacao_recebida = False

    while time.time() - start_time < timeout:
        if ser.in_waiting > 0:
            try:
                # Leia a linha completa ou os bytes disponíveis
                linha = ser.readline().decode('ascii').strip()
                if linha:
                    print(f"Mensagem recebida: {linha}")
                    # Compare a mensagem recebida com o comando enviado
                    if linha == codigo_enviado:
                        confirmacao_recebida = True
            except UnicodeDecodeError:
                # Ignore mensagens que não podem ser decodificadas
                pass
        time.sleep(0.1)  # Pequeno atraso para evitar uso excessivo da CPU
    
    if confirmacao_recebida:
        print("Confirmação recebida do receptor.")
    else:
        print("Tempo esgotado: Nenhuma confirmação foi recebida do receptor.")

    # Aguarda até completar 5 segundos desde o envio para voltar ao menu
    remaining_time = timeout - (time.time() - start_time)
    if remaining_time > 0:
        time.sleep(remaining_time)

def main():
    # Verifica se a porta serial foi passada como argumento
    if len(sys.argv) > 1:
        porta_serial = sys.argv[1]
    else:
        porta_serial = SERIAL_PORT

    # Inicializa a conexão serial
    try:
        ser = serial.Serial(porta_serial, BAUD_RATE, timeout=1)
        print(f"Conectado à porta {porta_serial} a {BAUD_RATE} baud.")
    except serial.SerialException as e:
        print(f"Erro ao abrir a porta serial {porta_serial}: {e}")
        sys.exit(1)

    time.sleep(2)  # Tempo para estabilizar a conexão

    try:
        while True:
            exibir_menu()
            selecao = obter_selecao()
            if selecao == 'sair':
                print("Encerrando o programa.")
                break
            descricao = OPCOES[selecao]['descricao']
            codigo = OPCOES[selecao]['codigo']
            if confirmar_execucao(descricao):
                enviar_comando(ser, codigo)
                # Espera pela confirmação
                esperar_confirmacao(ser, codigo, timeout=5)
            else:
                print("Opção cancelada pelo usuário.")
    except KeyboardInterrupt:
        print("\nPrograma interrompido pelo usuário.")
    finally:
        if ser.is_open:
            ser.close()
            print("Porta serial fechada.")

if __name__ == "__main__":
    main()
