import os
from imgproc import processImage
from PIL import Image
import time

word_list:list[str] = []
words_by_length:list[list[str]] = []
prefix_by_length:list[list[str]] = []

def solve(board: list[str]):
	global word_list, words_by_length, prefix_by_length

	shape = (len(board),len(board[0]))
	if any(map(lambda a: len(a) != shape[1],board)): raise IndexError(f"Board is not a rectangle!")
	board = list(map(str.lower,board))

	if len(word_list) == 0:
		word_list=genwordlist([os.path.join("wordslists","Collins Scrabble Words (2019).txt")],shape)
	
	words = prune_words(word_list,board)
	maxlen = max(map(len,words))

	words_by_length = [[] for _ in range(maxlen+1)]
	prefix_by_length = [[] for _ in range(maxlen+1)]

	for word in words:
		words_by_length[len(word)].append(word)

	for prefix_length in range(maxlen+1):
		working = []
		for word_length in range(prefix_length,maxlen+1):
			working.extend(map(lambda a: a[:prefix_length],words_by_length[word_length]))
		prefix_by_length[prefix_length] = list(set(working))
	paths = find_valid(board)
	return wash(paths)

def genwordlist(wordlists: list[str],board_size:(int|tuple[int,int])=16)->list[str]:
	'''
	Generates a combined wordlist from a number of existing ones

	Arguments:
		`wordslists` -- A list of paths to get words lists from
		`board_size` -- The size of the board, either in dimensions or absolute amount of cells
	'''

	board_size:int = int(board_size[0]*board_size[1]) if type(board_size) is tuple else board_size
	if not all(map(os.path.exists,wordlists)): raise FileNotFoundError("At least one wordlist not found")

	all_words = []
	for filename in wordlists:
		with open(filename) as f: all_words.extend(f.readlines())
	words_of_length = list(map(str.lower,filter(lambda a: len(a) <= board_size and len(a) >= 3, set(map(str.strip,all_words)))))

	print(f"Found {len(all_words)} words\nShortened to {len(words_of_length)} valid.")

	return words_of_length

def prune_words(wordlist:list[str], board:list[str]):
	included_letters = set(list("".join(board)))
	disallowed = set("abcdefghijklmnopqrstuvwxyz").difference(included_letters)

	good_words=[]
	
	for word in wordlist:
		ok=True
		for c in disallowed:
			if c not in word: continue
			ok=False
			break
		if ok: good_words.append(word)
	print(f"Pruned to {len(good_words)} on board")
	return good_words

class Path:
	def __init__(self,board:list[str],parent:Path|None=None,next_point:tuple[int,int]|None=None):
		self.path:list[tuple[int,int]] = []
		self.word = ""
		self.board=board
		if parent:
			self.path = parent.path.copy()
			self.word = parent.word
		
		if next_point:
			self.path.append(next_point)
			self.word += board[next_point[0]][next_point[1]]
	def is_word(self)->bool:
		global words_by_length
		return self.word in words_by_length[len(self.word)]
	def is_prefix(self)->bool:
		global prefix_by_length
		return self.word in prefix_by_length[len(self.word)]
	def __len__(self):
		return len(self.path)
	def __gt__(self,other):
		if type(other) != Path: return True
		return len(self) > len(other)
	def __lt__(self,other):
		if type(other) != Path: return False
		return len(other) < len(self)
	def __eq__(self,other):
		if type(other) == str and other == self.word: return True
		if type(other) != Path: return False
		return self.word == other.word
	def __hash__(self):
		return self.word.__hash__()
	def __str__(self):
		return self.word

def find_valid(board:list[str]):
	shape = (len(board),len(board[0]))

	def in_bounds(x:int,y:int):
		return not (x >= shape[0] or x<0 or y>= shape[1] or y<0)

	words:list[Path] = []

	for i in range(shape[0]):
		for j in range(shape[1]):
			start = (i,j)
			todo:list[Path] = []
			todo.append(Path(board,next_point=start))

			while len(todo):
				curr = todo.pop()
				if curr.is_word(): words.append(curr)
				x,y = curr.path[-1]

				if in_bounds(x-1,y-1) and (x-1,y-1) not in curr.path:
					neighbor = Path(board,curr,(x-1,y-1))
					if neighbor.is_prefix(): todo.append(neighbor)

				if in_bounds(x-1,y) and (x-1,y) not in curr.path:
					neighbor = Path(board,curr,(x-1,y))
					if neighbor.is_prefix(): todo.append(neighbor)

				if in_bounds(x-1,y+1) and (x-1,y+1) not in curr.path:
					neighbor = Path(board,curr,(x-1,y+1))
					if neighbor.is_prefix(): todo.append(neighbor)

				if in_bounds(x,y+1) and (x,y+1) not in curr.path:
					neighbor = Path(board,curr,(x,y+1))
					if neighbor.is_prefix(): todo.append(neighbor)

				if in_bounds(x+1,y+1) and (x+1,y+1) not in curr.path:
					neighbor = Path(board,curr,(x+1,y+1))
					if neighbor.is_prefix(): todo.append(neighbor)

				if in_bounds(x+1,y) and (x+1,y) not in curr.path:
					neighbor = Path(board,curr,(x+1,y))
					if neighbor.is_prefix(): todo.append(neighbor)

				if in_bounds(x+1,y-1) and (x+1,y-1) not in curr.path:
					neighbor = Path(board,curr,(x+1,y-1))
					if neighbor.is_prefix(): todo.append(neighbor)

				if in_bounds(x,y-1) and (x,y-1) not in curr.path:
					neighbor = Path(board,curr,(x,y-1))
					if neighbor.is_prefix(): todo.append(neighbor)
	words = list(set(words))
	words.sort()
	return words
	# print(*words)
	# print(f"Found {len(words)} valid paths")

def wash(paths:list[Path]) -> list[list[tuple[int,int]]]:
	'''Sanitize `paths` into a list of lists of tuples'''
	out = []
	for path in paths:
		out.append(path.path)
	return out

if __name__ == "__main__":
	start0 = time.time_ns()
	board = processImage(Image.open("test.png"))
	print(f"{(time.time_ns()-start0)/1e6}ms to cv/load")
	start1=time.time_ns()
	solve(board)
	print(f"{(time.time_ns()-start1)/1e6}ms to find")
	print(f"{(time.time_ns()-start0)/1e6}ms total")
