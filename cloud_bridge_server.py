import os
import io
import requests
import json
from flask import Flask, request, jsonify, render_template, send_from_directory
from werkzeug.utils import secure_filename
from PIL import Image
app = Flask(__name__)

# ==========================================
# Configurações do Discord Webhook
# ==========================================
DISCORD_WEBHOOK_URL = "WEBHOOK"

# ==========================================
# Configurações do Banco de Faces Locais
# ==========================================
KNOWN_FACES_DIR = "known_faces"

known_face_encodings = []
known_face_names = []

def load_known_faces():
    """MOCK: Carrega as faces salvas no diretório known_faces para a memória."""
    global known_face_names
    known_face_names = []
    
    if not os.path.exists(KNOWN_FACES_DIR):
        os.makedirs(KNOWN_FACES_DIR)
        print(f"Diretório {KNOWN_FACES_DIR} criado.")

    print("Carregando banco de faces conhecidas (MOCK)...")
    
    for filename in os.listdir(KNOWN_FACES_DIR):
        if filename.lower().endswith(('.jpg', '.jpeg', '.png')):
            name = os.path.splitext(filename)[0]
            known_face_names.append(name)
            print(f"  - Pessoa carregada: {name}")

def send_to_discord(image_bytes, person_name, granted=False):
    """Envia a captura e o resultado para o Discord configurado via Webhook"""
    if not DISCORD_WEBHOOK_URL or "SEU_WEBHOOK_URL_AQUI" in DISCORD_WEBHOOK_URL:
        print("Webhook não configurado. Ignorando envio para Discord.")
        return

    status_text = f"Acesso Liberado - {person_name}" if granted else "Acesso Negado - Desconhecido"
    color = 0x00FF00 if granted else 0xFF0000

    payload = {
        "embeds": [{
            "title": "Controle de Acesso API",
            "description": status_text,
            "color": color,
            "image": {
                "url": "attachment://capture.jpg"
            }
        }]
    }

    files = {
        # O discord webhook requer essa chave 'payload_json' e os 'files' pra imagem
        "payload_json": (None, json.dumps(payload)), 
        "file": ("capture.jpg", image_bytes, "image/jpeg")
    }

    try:
        req = requests.post(DISCORD_WEBHOOK_URL, files=files)
        if req.status_code in [200, 204]:
            print(f"  => Notificação enviada para o Discord com sucesso.")
        else:
            print(f"  => Falha ao enviar ao Discord: {req.status_code}")
    except Exception as e:
         print(f"  => Erro de conexão com o Discord: {e}")

@app.route('/', methods=['GET'])
def index():
    return render_template('index.html')

@app.route('/api/users', methods=['GET'])
def get_users():
    users = []
    if os.path.exists(KNOWN_FACES_DIR):
        for filename in os.listdir(KNOWN_FACES_DIR):
            if filename.lower().endswith(('.jpg', '.jpeg', '.png')):
                # Remove the extension to get the name
                name = os.path.splitext(filename)[0]
                users.append({"name": name, "filename": filename})
    return jsonify(users)

@app.route('/api/users/<name>', methods=['DELETE'])
def delete_user(name):
    # Procure o arquivo com várias terminações possíveis
    extensions = ['.jpg', '.jpeg', '.png']
    deleted = False
    
    for ext in extensions:
        # A API envia o nome original (já decodificado de URL)
        # secure_filename pode alterar, então vamos testar seguro e direto se garantir a extensão
        safe_name = secure_filename(name)
        file_path = os.path.join(KNOWN_FACES_DIR, safe_name + ext)
        if os.path.exists(file_path):
            os.remove(file_path)
            deleted = True
            break # se deletou um, ótimo

    if deleted:
        # Atualiza a memória
        load_known_faces()
        return jsonify({"success": True, "message": f"Usuário '{name}' removido."}), 200
    else:
        return jsonify({"success": False, "error": "Usuário não encontrado."}), 404

@app.route('/api/enroll', methods=['POST'])
def enroll():
    if 'image' not in request.files or 'name' not in request.form:
        return jsonify({"success": False, "error": "Faltando imagem ou nome!"}), 400
    
    file = request.files['image']
    name = request.form['name']
    
    if file.filename == '' or not name:
        return jsonify({"success": False, "error": "Arquivo ou nome inválido!"}), 400

    # Salva a imagem na pasta known_faces (sempre como .jpg para o python)
    # secure_filename vai remover espaços, substitua espaços por underline
    sanitized_name = secure_filename(name.replace(" ", "_"))
    filename = sanitized_name + ".jpg"
    
    if not os.path.exists(KNOWN_FACES_DIR):
        os.makedirs(KNOWN_FACES_DIR)
        
    path = os.path.join(KNOWN_FACES_DIR, filename)
    file.save(path)
    
    # Recarrega a memória
    load_known_faces()
    
    return jsonify({"success": True, "message": f"Usuário '{name}' cadastrado com sucesso!"}), 200

@app.route('/recognize', methods=['POST'])
def recognize():
    print("----- Movimento Detectado pelo ESP32! Acionando Webcam Local -----")
    
    # Abrir a webcam do Mac/Logitech (0 é a padrão, pode ser 1 ou 2 dependendo de quantas existirem)
    import cv2
    cap = cv2.VideoCapture(0)
    
    # Aguardar a câmera aquecer
    import time
    time.sleep(1)
    
    ret, frame = cap.read()
    cap.release()
    
    if not ret or frame is None:
        print("  => Erro fatal: Não foi possível acessar a webcam do Mac.")
        return jsonify({"result": "ERROR_BAD_IMAGE"}), 400

    # Converter o frame OpenCV (BGR) para JPG bytes para manter compatibilidade com o resto do código
    ret, buffer = cv2.imencode('.jpg', frame)
    if not ret:
        print("  => Erro ao codificar a imagem da webcam.")
        return jsonify({"result": "ERROR_BAD_IMAGE"}), 400
        
    image_bytes = buffer.tobytes()

    # --- MOCK LOGIC ---
    # A biblioteca face_recognition precisa compilar o dlib em C++, o que falhou no seu Mac.
    # Para testes, este mock vai aprovar ou negar aleatoriamente com base nas pessoas cadastradas.
    import random
    
    granted = random.choice([True, False])
    
    if granted and len(known_face_names) > 0:
        name = random.choice(known_face_names)
        print(f"  => Identificação (MOCK): {name} (Acesso Liberado)")
    else:
        name = "Desconhecido"
        granted = False
        print("  => Identificação (MOCK): Desconhecido (Acesso Negado)")
        
    send_to_discord(image_bytes, name, granted)
    
    if granted:
        return jsonify({"result": "GRANTED", "name": name}), 200
    else:
        return jsonify({"result": "DENIED", "name": "Unknown"}), 200


if __name__ == '__main__':
    load_known_faces()
    # Inicia o servidor na porta 5001 acessível na rede local
    app.run(host='0.0.0.0', port=5001, debug=False)
