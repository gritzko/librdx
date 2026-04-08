using System;
using System.Collections.Generic;
using System.Linq;

interface IPrintable {
    void Print();
}

record Point(double X, double Y);

enum Color { Red, Green, Blue }

class Animal : IPrintable {
    public string Name { get; set; }
    protected int Age;

    public Animal(string name, int age) {
        Name = name;
        Age = age;
    }

    public virtual void Print() {
        Console.WriteLine($"{Name} age={Age}");
    }

    public static Animal Create(string name) {
        return new Animal(name, 0);
    }
}

class Dog : Animal {
    private string breed;

    public Dog(string name, int age, string breed)
        : base(name, age) {
        this.breed = breed;
    }

    public override void Print() {
        Console.WriteLine($"{Name} [{breed}]");
    }
}

class Program {
    static List<string> Process(List<string> items) {
        return items
            .Where(s => !s.StartsWith("#"))
            .Select(s => s.Trim())
            .ToList();
    }

    static void Main(string[] args) {
        var dog = new Dog("Rex", 5, "Lab");
        dog.Print();
        var p = new Point(1.0, 2.0);
        Console.WriteLine(p);
        var colors = new List<Color> { Color.Red, Color.Blue };
        Console.WriteLine(Process(new List<string> { "a", "#b" }));
    }
}
