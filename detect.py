from transformers import ViTForImageClassification, ViTImageProcessor
from PIL import Image

# Load the model and image processor
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

# Function to classify an image from a local file
def classify_image(image_path):
    # Load image from local file
    image = Image.open(image_path)

    # Prepare the image for the model
    inputs = processor(images=image, return_tensors="pt")

    # Generate classification
    outputs = model(**inputs)
    logits = outputs.logits
    predicted_class_idx = logits.argmax(-1).item()
    
    # Retrieve label using the manual id2label mapping
    label = id2label.get(str(predicted_class_idx), f"Label not found for index {predicted_class_idx}")

    return label

# Example usage
image_path = "metal_can.jpg"  # Replace with the path to your local image file
image_path_2 = "can_test.jpg"
image_path_3 = "bottle_test.jpg"
image_path_4 = "cardboard_test.jpg"
result = classify_image(image_path)
print("Classification:", result)
result = classify_image(image_path_2)
print("Classification:", result)
result = classify_image(image_path_3)
print("Classification:", result)
result = classify_image(image_path_4)
print("Classification:", result)


