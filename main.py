from PIL import Image
import io
from typing import Dict, Any
import base64
from scrabblesolve import solve
from imgproc import processImage
import json

def handler(img_data:bytes) -> str:
	img = Image.open(io.BytesIO(img_data))
	grid = processImage(img)
	img.close()
	paths = solve(grid)

	return json.dumps(paths)

def lambda_handler(event:Dict[str,Any], context:Any)->Dict[str,Any]:
	method = event["http"]["method"]
	
	if method != "POST": return { 'statusCode': 400, 'body': "Hiya! Please post instead :)" }
	body:str = event["body"]
	img_data = base64.b64decode(body)
	
	return {
		'statusCode': 200,
		'body': handler(img_data)
	}