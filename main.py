from flask import Flask, request
from PIL import Image
import io
import numpy as np
from imgproc import processImage

app = Flask(__name__)

@app.route("/")
def hello():
	return "Hello."

@app.route("/",methods=["POST"])
def recieveImage():
	if "image/" not in request.headers["Content-Type"]:
		return "", 400
	processImage(Image.open(io.BytesIO(request.data),'r'))
	
	return "Hooray!"

if __name__ == "__main__":
	app.run()