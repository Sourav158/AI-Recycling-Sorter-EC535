from transformers import ViTForImageClassification, ViTImageProcessor
from PIL import Image
import cv2
import datetime

# Load the model and image processor only once
model = ViTForImageClassification.from_pretrained("hagerty7/recyclable-materials-classification")
processor = ViTImageProcessor.from_pretrained("hagerty7/recyclable-materials-classification")

# Define a manual label mapping
id2label = {
    '0': 'cardboard',
    '1': 'glass',
    '2': 'metal',
    '3': 'paper',
    '4': 'plastic',
    '5': 'trash'
}

# Function to check available cameras and capture and classify an image from a specified camera index
def classify():
    # Capture and classify image using the selected camera index
    cap = cv2.VideoCapture(0)
    try:
        if not cap.isOpened():
            raise IOError("Cannot open webcam")

        # Capture one frame
        ret, frame = cap.read()
        if not ret:
            print("Failed to grab frame")
            return

        # Optionally resize the image to reduce processing time
        frame = cv2.resize(frame, (224, 224), interpolation=cv2.INTER_AREA)

        # Save the captured image
        timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"captured_image_{timestamp}.jpg"
        cv2.imwrite(filename, frame)

        # Convert the captured frame to PIL Image
        image = Image.fromarray(cv2.cvtColor(frame, cv2.COLOR_BGR2RGB))

        # Prepare the image for the model
        inputs = processor(images=image, return_tensors="pt")

        # Generate classification
        outputs = model(**inputs)
        logits = outputs.logits
        predicted_class_idx = logits.argmax(-1).item()
        
        # Retrieve label using the manual id2label mapping
        label = id2label.get(str(predicted_class_idx), f"Label not found for index {predicted_class_idx}")

        print("Classification:", label)
        print("Image saved as:", filename)
    finally:
        cap.release()

# Run the function
classify()
