
from flask import Flask, request
from flask_cors import CORS
import json
from groq import Groq

app = Flask(__name__)
CORS(app)


GROQ_API_KEY = ""
client_groq = Groq(api_key=GROQ_API_KEY)


try:
    with open('meniu.json', 'r', encoding='utf-8') as f:
        MENIU = json.load(f)
except Exception as e:
    print(f"Eroare la încărcarea meniului: {e}")
    MENIU = []

def calculeaza_scor(preparat, profil):
    scor = 100
    penalizari = []
    bonusuri = []

    # Filtre drastice (Eliminare directă)
    if profil.get('faraGluten') and not preparat.get('fara_gluten'): return 0, []
    if profil.get('vegetarian') and not preparat.get('vegetarian'): return 0, []
    if profil.get('vegan') and not preparat.get('vegan'): return 0, []
    
    # Verificare Alergii
    alergeni_preparat = preparat.get('alergeni', [])
    if profil.get('alergiiNuci') and 'nuci' in alergeni_preparat: return 0, []
    if profil.get('alergiiPeste') and 'peste' in alergeni_preparat: return 0, []
    if profil.get('alergiiLactate') and 'lactate' in alergeni_preparat: return 0, []

    # Afecțiuni medicale
    afectiuni_client = []
    if profil.get('diabet'): afectiuni_client.append('diabet')
    if profil.get('hipertensiune'): afectiuni_client.append('hipertensiune')
    if profil.get('colesterol'): afectiuni_client.append('colesterol')

    contraindicate = preparat.get('afectiuni_contraindicate', [])
    recomandate = preparat.get('afectiuni_recomandate', [])

    for afectiune in afectiuni_client:
        if afectiune in contraindicate:
            scor -= 30
            penalizari.append(afectiune)
        if afectiune in recomandate:
            scor += 15
            bonusuri.append(f'recomandat pentru {afectiune}')

    # Preferințe bucătărie
    bucatarie_pref = profil.get('bucatariePreferata', '').lower()
    bucatarii_map = {
        'română': 'romaneasca', 'italiană': 'italiana', 'asiatică': 'asiatica',
        'americană': 'americana', 'mediteraneană': 'mediteraneana', 'franceză': 'franceza'
    }
    bucatarie_cod = bucatarii_map.get(bucatarie_pref, '')
    if bucatarie_cod and preparat.get('tip_bucatarie') == bucatarie_cod:
        scor += 20
        bonusuri.append(f'bucătărie {bucatarie_pref}')

    # Picant
    if profil.get('picant') and preparat.get('picant'):
        scor += 10
        bonusuri.append('picant')
    elif not profil.get('picant') and preparat.get('picant'):
        scor -= 10

    return max(0, min(100, scor)), bonusuri + [f'penalizat: {p}' for p in penalizari]

def genereaza_explicatie_groq(preparat, scor, motive, profil):
    try:
        prompt = f"""Ești un asistent nutriționist la un restaurant. 
Generează o explicație scurtă (maxim 2 propoziții) în română de ce preparatul "{preparat['nume']}" 
cu scorul {scor}% este potrivit pentru un client cu următorul profil:
- Afecțiuni: {'diabet' if profil.get('diabet') else ''} {'hipertensiune' if profil.get('hipertensiune') else ''} {'colesterol' if profil.get('colesterol') else ''}
- Preferințe: {'vegetarian' if profil.get('vegetarian') else ''} {'vegan' if profil.get('vegan') else ''} {'fără gluten' if profil.get('faraGluten') else ''}
Motive calcul scor: {', '.join(motive) if motive else 'preparat neutru'}
Răspunde DOAR cu explicația, fără alte cuvinte."""

        completion = client_groq.chat.completions.create(
            model="llama-3.1-8b-instant",
            messages=[{"role": "user", "content": prompt}],
            max_tokens=150,
            temperature=0.7
        )
        return completion.choices[0].message.content.strip()
    except Exception as e:
        print(f"Eroare Groq pentru {preparat['nume']}: {e}")
        return f"Acest preparat are un scor de {scor}% bazat pe profilul tău de sănătate."

@app.route('/recommend', methods=['POST'])
def recommend():
    profil = request.json.get('clientProfile', {})
    
    toate_rezultatele = []
    for preparat in MENIU:
        scor, motive = calculeaza_scor(preparat, profil)
        if scor >= 50:
            toate_rezultatele.append({
                'id': preparat['id'],
                'nume': preparat['nume'],
                'categorie': preparat['categorie'],
                'pret': preparat['pret'],
                'calorii': preparat['calorii'],
                'scor': scor,
                'preparat': preparat,
                'motive': motive
            })
    
    # --- LOGICA DE DIVERSIFICARE (CÂTE 3 DIN FIECARE) ---
    categorii_tinta = ['Supe/Ciorbe', 'Salate', 'Fel principal (carne)', 'Paste/Pizza', 'Deserturi', 'Băuturi']
    final_recomandari = []

    for cat in categorii_tinta:
        # Filtrăm și sortăm preparatele din categoria curentă
        preparate_cat = [r for r in toate_rezultatele if r['categorie'] == cat]
        preparate_cat.sort(key=lambda x: x['scor'], reverse=True)
        
        # Luăm top 3 din această categorie
        final_recomandari.extend(preparate_cat[:3])

    # Generăm explicații AI pentru selecția diversificată
    for r in final_recomandari:
        r['explicatie'] = genereaza_explicatie_groq(r['preparat'], r['scor'], r['motive'], profil)
        r.pop('motive', None) # Ștergem motivele brute înainte de trimitere

    combinatie = genereaza_combinatie(toate_rezultatele, profil)
    
    return app.response_class(
        response=json.dumps({
            'recomandari': final_recomandari,
            'combinatie_recomandata': combinatie
        }, ensure_ascii=False, indent=2),
        status=200,
        mimetype='application/json'
    )

def genereaza_combinatie(rezultate, profil):
    combinatie = {}
    categorii = ['Supe/Ciorbe', 'Salate', 'Fel principal (carne)', 'Paste/Pizza', 'Deserturi', 'Băuturi']
    
    for categorie in categorii:
        preparate_categorie = [r for r in rezultate if r['categorie'] == categorie]
        if preparate_categorie:
            # Sortăm pentru a asigura că cel mai bun scor din categorie e în combinație
            preparate_categorie.sort(key=lambda x: x['scor'], reverse=True)
            combinatie[categorie] = preparate_categorie[0]
    
    calorii_totale = sum(c['calorii'] for c in combinatie.values())
    
    return {
        'preparate': combinatie,
        'calorii_totale': calorii_totale
    }

@app.route('/health', methods=['GET'])
def health():
    return app.response_class(
        response=json.dumps({'status': 'ok'}, ensure_ascii=False),
        status=200,
        mimetype='application/json'
    )

if __name__ == '__main__':
    app.run(host="0.0.0.0", port=5001, debug=True)