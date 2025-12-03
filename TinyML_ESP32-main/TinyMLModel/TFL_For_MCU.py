import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import MinMaxScaler
import tensorflow as tf
print(tf.__version__)

PREFIX = "TinyML"


data = pd.read_csv("sensor_data.csv")
X = data[["temperature", "humidity", "light", "moisture"]].values
# scaler = MinMaxScaler()
# X = scaler.fit_transform(X)

# y = data["system_state"].values
y = (data["system_state"] == "Normal").astype(int).values

X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2)

# Simple classifier model
model = tf.keras.Sequential([
    tf.keras.layers.Input(shape=(4,)),
    tf.keras.layers.Dense(8, activation='relu'),
    tf.keras.layers.Dense(4, activation='relu'),
    tf.keras.layers.Dense(1, activation='sigmoid')
])

# model.compile(loss="binary_crossentropy", optimizer="adam", metrics=["accuracy"])
model.compile(loss="binary_crossentropy", optimizer="adam", metrics=[
    "accuracy",
    tf.keras.metrics.Precision(),
    tf.keras.metrics.Recall()
])

callback = tf.keras.callbacks.EarlyStopping(patience=5, restore_best_weights=True)

model.fit(X_train, y_train, epochs=50, validation_data=(X_test, y_test), callbacks =[callback])
model.save(PREFIX + '.h5')


# Convert to TFLite
converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]
tflite_model = converter.convert()

with open(PREFIX + ".tflite", "wb") as f:
    f.write(tflite_model)

tflite_path = PREFIX + '.tflite'
output_header_path = PREFIX + '.h'

with open(tflite_path, 'rb') as tflite_file:
    tflite_content = tflite_file.read()

hex_lines = [', '.join([f'0x{byte:02x}' for byte in tflite_content[i:i + 12]]) for i in
         range(0, len(tflite_content), 12)]

hex_array = ',\n  '.join(hex_lines)

with open(output_header_path, 'w') as header_file:
    header_file.write('const unsigned char model[] = {\n  ')
    header_file.write(f'{hex_array}\n')
    header_file.write('};\n\n')