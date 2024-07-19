import json
import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt #pour créer des graphes
from sklearn.model_selection import train_test_split, learning_curve
from sklearn.preprocessing import StandardScaler #pour la normalisation des données
from sklearn.metrics import classification_report, accuracy_score, confusion_matrix
import tensorflow as tf
from tensorflow.keras.models import load_model
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense
from tensorflow.keras.utils import to_categorical

csv_path = 'C:\\Users\\DELL\\Desktop\\stage iot\\infos stage4A\\Code_modèle\\test_final_cap3_labeled.csv'
data = pd.read_csv(csv_path,  sep=';')

# Ajouter la colonne 'Label' en fonction de 'label_tag'
def assign_label(label_tag):
    if label_tag in [0, 1]:
        return 'normal air'
    elif label_tag == 2:
        return 'fumee'
    else:
        return 'unknown'

data['label'] = data['Label Tag'].apply(assign_label)

output_csv_path = 'C:\\Users\\DELL\\Desktop\\stage iot\\infos stage4A\\Code_modèle\\test_final_cap4_labeled.csv'
data.to_csv(output_csv_path, sep=';', index=False)
print(f"Data saved to CSV at {output_csv_path}")

# Charger les données
data = pd.read_csv(output_csv_path,  sep=';')

# Vérifier si la colonne 'label' existe
if 'label' not in data.columns:
    raise ValueError("Le fichier CSV doit contenir une colonne 'label'.")
print("'label' column exists.")

# Prétraitement des données

selected_columns = ['Temperature', 'Pressure', 'Relative Humidity','Resistance Gassensor']
X = data[selected_columns]
y = data['label']  # Étiquettes

# Convertir les étiquettes en format one-hot encoding
y = pd.get_dummies(y).values

# Diviser les données en ensembles d'entraînement (80%) et de test (20%) à l'aide de la fonction train_test_split
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)
print("First 5 rows of X_train:")
print(X_train.head())

# Normalisation des données
scaler = StandardScaler()
X_train_scaled = scaler.fit_transform(X_train)
X_test_scaled = scaler.transform(X_test)
# Convertir X_train_scaled en DataFrame pour l'affichage
X_train_scaled_df = pd.DataFrame(X_train_scaled, columns=X_train.columns)
# Afficher les premières lignes de X_train_scaled
print("\nFirst 5 rows of X_train_scaled:")
print(X_train_scaled_df.head())

# Construction du modèle de réseau de neurones

model = Sequential()
model.add(Dense(64, input_dim=X_train_scaled.shape[1], activation='relu'))
model.add(Dense(64, activation='relu'))
model.add(Dense(y.shape[1], activation='softmax'))  # Nombre de classes dans y

model.compile(loss='categorical_crossentropy', optimizer='adam', metrics=['accuracy'])

# Entraînement du modèle
history = model.fit(X_train_scaled, y_train, validation_data=(X_test_scaled, y_test), epochs=50, batch_size=32)

# Évaluation du modèle
loss, accuracy = model.evaluate(X_test_scaled, y_test)
print(f'Accuracy: {accuracy}')

# Prédictions
y_pred = model.predict(X_test_scaled)
y_pred_classes = np.argmax(y_pred, axis=1)
y_test_classes = np.argmax(y_test, axis=1)

print(classification_report(y_test_classes, y_pred_classes))

# Calcul de la matrice de confusion
conf_matrix = confusion_matrix(y_test_classes, y_pred_classes)
print("Confusion Matrix:")
print(conf_matrix)

# Visualisation de la matrice de confusion
plt.figure(figsize=(10,7))
sns.heatmap(conf_matrix, annot=True, fmt='d', cmap='Blues', xticklabels=data['label'].unique(), yticklabels=data['label'].unique())
plt.xlabel('Predicted')
plt.ylabel('Actual')
plt.title('Confusion Matrix')
plt.show()

# Visualisation de la courbe de perte

plt.figure(figsize=(10, 7))
plt.plot(history.history['loss'], label='Training Loss')
plt.plot(history.history['val_loss'], label='Validation Loss')
plt.xlabel('Epochs')
plt.ylabel('Loss')
plt.title('Loss Curves')
plt.legend()
plt.show()
