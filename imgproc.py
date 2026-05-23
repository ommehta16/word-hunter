from PIL import Image
import numpy as np
import cv2
import pytesseract
import functools

def processImage(img:Image.Image):
	im = cv2.cvtColor(np.array(img),cv2.COLOR_RGB2BGR)
	
	threshed = cv2.extractChannel(im,2)
	threshed = cv2.convertScaleAbs(threshed,alpha=10)
	ret, threshed = cv2.threshold(threshed,127,255,cv2.THRESH_BINARY)

	contours, hierarchy = cv2.findContours(threshed,cv2.RETR_TREE,cv2.CHAIN_APPROX_SIMPLE)

	out=cv2.cvtColor(threshed,cv2.COLOR_GRAY2BGR)

	HEIGHT = out.shape[0]
	WIDTH = out.shape[1]

	area = lambda rect: abs((rect[0]-rect[1]) * (rect[2]-rect[3]))
	
	rects = list(map(cv2.boundingRect,contours))

	cmp = lambda a,b: area(cv2.boundingRect(a))-area(cv2.boundingRect(b))
	contours.sort(key=functools.cmp_to_key(cmp))
	if len(contours) < 16:
		print("WHAT?!?")
		raise IndexError("Not enough letters detected; board should have atleast 16")
	contours = contours[:16]

	for contour in contours:
		x,y,w,h = cv2.boundingRect(contour)
		if w > 0.2*WIDTH or h > 0.2*HEIGHT: continue
		if y < 0.3*HEIGHT: continue
		# print(x,y,x+w,y+h)
		# print(WIDTH,HEIGHT)
		if x+w > WIDTH or y+h > HEIGHT: continue
		text = str(pytesseract.image_to_string(threshed[y-20:y+h+20,x-20:x+w+20],lang="eng",config=r'--oem 3 --psm 6'))
		print(ord(text[1]), len(text))
		cv2.rectangle(out,(x-20,y-20),(x+w+20,y+h+20),(0,255,0),2)
		cv2.putText(out,text,(x,y),cv2.FONT_HERSHEY_SIMPLEX,1,(0,0,0),1)
	# out = cv2.drawContours(cv2.cvtColor(threshed,cv2.COLOR_GRAY2BGR),contours,-1,(0,255,0),5)

	cv2.imshow("hallo",out)
	cv2.waitKey(10000)
	
	Image.fromarray(cv2.cvtColor(out,cv2.COLOR_BGR2RGB)).save("img.png")

if __name__ == "__main__":
	processImage(Image.open("test.png"))
	print("hooray!")