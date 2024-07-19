import json
import pandas as pd

# Lire le fichier texte contenant les données JSON
with open('C:\\Users\\DELL\\Desktop\\progr\\data_air_cap3\\2024_04_19_13_24_Board_F412FA67030C_PowerOnOff_1_hlzwoxcnc3g8v3pg_File_1.bmerawdata', 'r') as file:
    data = json.load(file)

# Extraire les en-têtes des colonnes
columns = [column['name'] for column in data['rawDataBody']['dataColumns']]

# Extraire les blocs de données
data_blocks = data['rawDataBody']['dataBlock']

# Convertir les données en DataFrame
df = pd.DataFrame(data_blocks, columns=columns)

# Remplacer les 0 dans l'avant-dernière colonne par 1
df.iloc[:, -2] = df.iloc[:, -2].replace(0, 1)

# Écrire les données dans un fichier CSV
df.to_csv('air_capt3.csv', index=False, sep=';')

print("Le fichier CSV a été généré avec succès.")
