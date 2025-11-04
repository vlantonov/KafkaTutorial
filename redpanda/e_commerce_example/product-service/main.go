package main

import (
    "encoding/json"
    "log"
    "net/http"
)

type Product struct {
    ID    string  `json:"id"`
    Name  string  `json:"name"`
    Price float64 `json:"price"`
}

func main() {
    http.HandleFunc("/products", func(w http.ResponseWriter, r *http.Request) {
        products := []Product{
            {ID: "1", Name: "Product 1", Price: 19.99},
            {ID: "2", Name: "Product 2", Price: 29.99},
        }
        json.NewEncoder(w).Encode(products)
    })

    log.Fatal(http.ListenAndServe(":8080", nil))
}
