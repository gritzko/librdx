import os
from typing import List, Optional

class Animal:
    """Base class for animals."""

    def __init__(self, name: str, age: int):
        self.name = name
        self.age = age

    def speak(self) -> str:
        return f"{self.name} says hello"

    def __repr__(self) -> str:
        return f"Animal({self.name!r}, {self.age})"

class Dog(Animal):
    def __init__(self, name: str, age: int, breed: str):
        super().__init__(name, age)
        self.breed = breed

    def speak(self) -> str:
        return f"{self.name} barks"

def greet(name: str) -> str:
    if not name:
        return "Hello, stranger!"
    return f"Hello, {name}!"

def process(items: List[str]) -> List[str]:
    result = []
    for item in items:
        if item.startswith('#'):
            continue
        result.append(item.strip())
    return result

def find(haystack: str, needle: str) -> Optional[int]:
    try:
        return haystack.index(needle)
    except ValueError:
        return None

if __name__ == "__main__":
    dog = Dog("Rex", 5, "Labrador")
    print(dog.speak())
    print(greet("World"))
    print(process(["a", "# b", " c "]))
