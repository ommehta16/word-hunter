from PIL import Image
import numpy as np
import cv2
import pytesseract
import functools

# (`height`,`width`)
GRID_SIZE = (4,4)
TOTAL_LETTERS = GRID_SIZE[0]*GRID_SIZE[1]

def processImage(img:Image.Image):
	im = cv2.cvtColor(np.array(img),cv2.COLOR_RGB2BGR)

	grid = extract_grid(im)
	print(*grid,sep="\n")

def extract_grid(im:cv2.typing.MatLike):
	threshed = cv2.extractChannel(im,2)
	threshed = cv2.convertScaleAbs(threshed,alpha=10)
	ret, threshed = cv2.threshold(threshed,127,255,cv2.THRESH_BINARY)
	contours, hierarchy = cv2.findContours(threshed,cv2.RETR_TREE,cv2.CHAIN_APPROX_SIMPLE)
	out=cv2.cvtColor(threshed,cv2.COLOR_GRAY2BGR)

	HEIGHT, WIDTH = threshed.shape[0], threshed.shape[1]

	def good_pos_and_size(rect):
		x,y,w,h = rect

		if w > 0.2*WIDTH or h > 0.2*HEIGHT: return False # i.e. too big
		if y < 0.3*HEIGHT: return False # i.e. too high
		if x+w > WIDTH or y+h > HEIGHT: return False # i.e. out of bounds
		return True
	
	area = lambda rect: abs(rect[2]*rect[3])
	cmp_area = lambda a,b: area(b)-area(a) # descending area-wise
	
	rects = list(map(cv2.boundingRect,contours))
	rects = filter(good_pos_and_size, rects)
	rects = sorted(rects, key=functools.cmp_to_key(cmp_area))

	if len(rects) < TOTAL_LETTERS:
		print("WHAT?!?")
		raise IndexError(f"Not enough letters detected; Detected {len(rects)}, board should have atleast {TOTAL_LETTERS}")
	
	rects = rects[:16]
	
	letterLocs = []
	for rect in rects:
		x,y,w,h = rect
		top, bottom = max(y-20,0), min(y+h+20,HEIGHT)
		left, right = max(x-20,0), min(x+w+20,WIDTH)

		text = str(pytesseract.image_to_string(threshed[top:bottom,left:right],lang="eng",config=r'--oem 3 --psm 6'))
		letter = text[0]
		# cv2.rectangle(out,(left,top),(right,bottom),(0,255,0),2) # DEBUG: outline detected characters' aabbs
		# cv2.putText(out,text,(x,y),cv2.FONT_HERSHEY_SIMPLEX,2,(0,0,255),2) # DEBUG: superimpose detected characters
		letterLocs.append(((y,x),letter))
	letterLocs.sort() # sorts from **top to bottom**
	annotatedGrid = [
		sorted(
			letterLocs[i*GRID_SIZE[1]:(i+1)*GRID_SIZE[1]],
			key=functools.cmp_to_key(lambda a,b: a[0][1]-b[0][1])
			) for i in range(GRID_SIZE[0])
	]
	# Image.fromarray(cv2.cvtColor(out,cv2.COLOR_BGR2RGB)).save("img.png") # DEBUG: output image
	# cv2.imshow("hallo",out)
	# cv2.waitKey(10000)
	grid = list(map(lambda a: "".join(map(lambda b: b[1],a)),annotatedGrid))

	return grid

if __name__ == "__main__":
	img = Image.open("test.png")
	processImage(img)
	print("hooray!")