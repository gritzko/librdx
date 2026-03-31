package main

import (
	"fmt"
	"strings"
)

type Point struct {
	X int
	Y int
}

type Stringer interface {
	String() string
}

const MaxSize = 1024

var globalCount int

func NewPoint(x, y int) *Point {
	return &Point{X: x, Y: y}
}

func (p *Point) Distance() float64 {
	return float64(p.X*p.X + p.Y*p.Y)
}

func (p *Point) String() string {
	return fmt.Sprintf("(%d, %d)", p.X, p.Y)
}

func process(items []string) []string {
	result := make([]string, 0, len(items))
	for _, item := range items {
		if strings.HasPrefix(item, "#") {
			continue
		}
		result = append(result, strings.TrimSpace(item))
	}
	return result
}

func main() {
	p := NewPoint(3, 4)
	fmt.Println(p.String())
	fmt.Println(p.Distance())
	data := []string{"hello", "# comment", " world "}
	fmt.Println(process(data))
}
