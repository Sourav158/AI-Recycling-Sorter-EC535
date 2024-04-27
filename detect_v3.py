import sys
import json
from transformers import ViTForImageClassification, ViTImageProcessor
from PIL import Image
import cv2
import datetime

model = ViTForImageClassification.from_pretrained("hagerty7/recyclable-materials-classification")
processor = ViTImageProcessor.from_pretrained("hagerty7/recyclable-materials-classification")

id2label = {
    '0': 'cardboard',
    '1': 'glass',
    '2': 'metal',
    '3': 'paper',
    '4': 'plastic',
    '5': 'trash'
}

def capture_and_classify_image():
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print(json.dumps({"error": "Cannot open webcam"}))
        cap.release()
        sys.exit(1)

    ret, frame = cap.read()
    cap.release()
    if not ret:
        print(json.dumps({"error": "Failed to grab frame"}))
        sys.exit(1)

    timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"public/captured_image_{timestamp}.jpg" 
    cv2.imwrite(filename, frame)
    
    image = Image.fromarray(cv2.cvtColor(frame, cv2.COLOR_BGR2RGB))
    inputs = processor(images=image, return_tensors="pt")
    outputs = model(**inputs)
    logits = outputs.logits
    predicted_class_idx = logits.argmax(-1).item()
    label = id2label.get(str(predicted_class_idx), "Label not found")

    print(json.dumps({"filename": filename, "label": label}))

if __name__ == "__main__":
    capture_and_classify_image()
