import serial
import numpy as np
import time

PORT     = "COM10"    
BAUDRATE = 115200

# classes 
CLASSES = ['HDF', 'No Failure', 'OSF', 'PWF', 'TWF']

# donnees de test
X_test = np.load("X_test_ai4i.npy").astype('float32')
y_test = np.load("y_test_ai4i.npy", allow_pickle=True)

print(f"Donnees chargees : {X_test.shape[0]} echantillons, {X_test.shape[1]} features")
print(f"Classes : {CLASSES}")
print()

ser = serial.Serial(PORT, BAUDRATE, timeout=5)
time.sleep(1)

correct = 0
n_test  = 50   # nbr echantillons a tester

print("=" * 65)
print("  TEST MAINTENANCE PREDICTIVE AI4I SUR STM32")
print("=" * 65)

for i in range(n_test):

    ser.write(bytes([0xAB, 0x00]))

    ack = ser.read(2)
    if len(ack) < 1 or ack[0] != 0xCD:
        print(f"[{i:02d}] Pas d'ACK ! recu: {ack.hex()}")
        continue

    ser.write(X_test[i].tobytes())

    raw = ser.read(5)
    if len(raw) < 5:
        print(f"[{i:02d}] Reponse incomplete")
        continue

    scores     = np.frombuffer(raw, dtype=np.uint8)
    predicted  = CLASSES[int(np.argmax(scores))]
    expected   = str(y_test[i])
    confidence = int(scores[np.argmax(scores)])

    if predicted == expected:
        correct += 1

    status = "OK  " if predicted == expected else "FAUX"
    print(f"[{i:02d}] Reel={expected:<12}  Predit={predicted:<12}  "
          f"{status}  Confiance={100*confidence/255:.0f}%")

ser.close()

precision = 100 * correct / n_test
print()
print("=" * 65)
print(f"  RESULTAT FINAL : {correct}/{n_test} correctes")
print(f"  PRECISION      : {precision:.1f}%")
if precision >= 80:
    print("  DECISION       : Modele valide sur STM32")
elif precision >= 60:
    print("  DECISION       : Resultats acceptables")
else:
    print("  DECISION       : Resultats insuffisants")
print("=" * 65)