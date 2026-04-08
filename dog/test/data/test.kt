interface Printable {
    fun print()
}

enum class Color { RED, GREEN, BLUE }

data class Point(val x: Double, val y: Double)

class Animal(val name: String, var age: Int) : Printable {
    override fun print() {
        println("$name age=$age")
    }

    fun greet(): String {
        return "Hello, I'm $name"
    }

    companion object {
        fun create(name: String): Animal {
            return Animal(name, 0)
        }
    }
}

class Dog(name: String, age: Int, val breed: String) : Animal(name, age) {
    override fun print() {
        println("$name [$breed]")
    }
}

fun process(items: List<String>): List<String> {
    return items
        .filter { !it.startsWith("#") }
        .map { it.trim() }
}

val DEFAULT_NAME = "World"
var counter = 0

fun greet(name: String = DEFAULT_NAME): String {
    counter++
    return "Hello, $name! (#$counter)"
}

fun main() {
    val dog = Dog("Rex", 5, "Lab")
    dog.print()
    println(greet())
    println(process(listOf("a", "# b", " c ")))
}
