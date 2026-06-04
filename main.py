from PIL import Image
import io
from typing import Dict, Any
import base64
from scrabblesolve import solve
from imgproc import processImage
import json
# https://www.serverless.com/plugins/serverless-wsgi#usage-without-serverless
import serverless_wsgi
from flask import Flask, request

app = Flask(__name__)

@app.route('/',methods=["POST","GET"])
def handle():
	if request.method == 'GET': return 'Hiya! Please POST instead :)', 400

	img_data = request.data

	img = Image.open(io.BytesIO(img_data))
	# img.save("test.png")
	grid = processImage(img)
	img.close()
	paths = solve(grid)

	''' holy convoluted
	Paths seperated by `_`
	Points seperated by `~`
	Coordinate components seperated by `.`
	
	'''
	compressed_text = "_".join(map(lambda k: "~".join(map(lambda p: ".".join(map(str,p)),k)),paths))

	return compressed_text, 200

def lambda_handler(event, context):
	return serverless_wsgi.handle_request(app,event,context)

if __name__ == "__main__":
	app.run('127.0.0.1',5000)